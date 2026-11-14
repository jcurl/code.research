#include "json_reader_context.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "rtest/rtest.h"
#include "json_reader_context_inputs.h"
#include "json_reader_mapped_file.h"
#include "test_output.h"

namespace valid_inputs = ubench::sjson::detail::valid_inputs;
namespace invalid_inputs = ubench::sjson::detail::invalid_inputs;
namespace invalid_grammar = ubench::sjson::detail::invalid_grammar;

namespace {

using token_list = std::vector<ubench::sjson::detail::json_reader_token>;

auto expect_keywords(ubench::sjson::detail::json_reader_context& context,
    const token_list& expected, bool expect_error = false) -> void {
  token_list actual;
  while (auto token = context.get_keyword()) {
    actual.push_back(*token);
  }
  ASSERT_EQ(actual.size(), expected.size());
  for (std::size_t i{}; i < expected.size(); ++i) {
    SCOPED_TRACE(i);
    EXPECT_EQ(actual[i].keyword(), expected[i].keyword());
    EXPECT_EQ(actual[i].value(), expected[i].value());
  }
  EXPECT_EQ(context.has_error(), expect_error);
}

}  // namespace

TEST(sjson_reader, context_get_keyword_empty_input_returns_nullopt) {
  ubench::sjson::detail::json_reader_context context{valid_inputs::empty};
  ASSERT_OPTIONAL_HAS_ERROR(context.get_keyword());
  EXPECT_FALSE(context.has_error());
}

TEST(sjson_reader, context_get_keyword_comment_only_returns_nullopt) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::only_comment};
  ASSERT_OPTIONAL_HAS_ERROR(context.get_keyword());
  EXPECT_FALSE(context.has_error());
}

TEST(sjson_reader, context_get_keyword_skips_comment_at_start) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::comment_at_start};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_skips_comment_in_middle) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::comment_in_middle};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_skips_comment_after_line) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::comment_after_line};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_line_endings_produce_same_tokens) {
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "a"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };

  ubench::sjson::detail::json_reader_context cr{valid_inputs::newlines_cr};
  expect_keywords(cr, expected);

  ubench::sjson::detail::json_reader_context crlf{valid_inputs::newlines_crlf};
  expect_keywords(crlf, expected);

  ubench::sjson::detail::json_reader_context lf{valid_inputs::newlines_lf};
  expect_keywords(lf, expected);
}

TEST(sjson_reader, context_get_keyword_ignores_whitespace) {
  ubench::sjson::detail::json_reader_context context{valid_inputs::whitespace};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "2"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "3"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_returns_punctuation) {
  ubench::sjson::detail::json_reader_context context{valid_inputs::punctuation};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "k"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_returns_keywords) {
  ubench::sjson::detail::json_reader_context context{valid_inputs::keywords};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::bool_true, "true"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::bool_false, "false"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::null_value, "null"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_returns_integers) {
  ubench::sjson::detail::json_reader_context context{valid_inputs::integers};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "0"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "42"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "-7"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer,
          "9223372036854775807"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_returns_string_contents_without_quotes) {
  ubench::sjson::detail::json_reader_context context{valid_inputs::strings};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, ""},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "abc"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "a b"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      // A "//" inside a string is content, not a comment.
      {ubench::sjson::detail::json_string_keyword::string, "http://x"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_plus_prefixed_integers) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::plus_prefixed_integers};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "+0"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "+42"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer,
          "+9223372036854775807"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_returns_utf8_strings) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::utf8_strings};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "\303\251"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "\360\237\230\200"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "a"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_array_of_integers_trailing_comma) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::array_of_integers_trailing_comma};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "2"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "3"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_object_of_integers_trailing_comma) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::object_of_integers_trailing_comma};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "a"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "b"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::integer, "2"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "c"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::integer, "3"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_string_with_escaped_quotes) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::escaped_quotes_in_string};
  // The escape sequences are returned as written, not unescaped.
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, R"(say \"hi\")"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_single_quoted_strings) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::single_quoted_strings};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "a"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "b c"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_string_with_escaped_backslash) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::escaped_backslash_in_string};
  // The backslash escape is returned as written, not unescaped.
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "a\\\\b"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_string_with_escaped_slash) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::escaped_slash_in_string};
  // The slash escape is returned as written, not unescaped.
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "a\\/b"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader,
    context_get_keyword_unescaped_double_quote_in_single_quoted_string) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::unescaped_double_quote_in_single_quoted};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "say \"hi\""},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader,
    context_get_keyword_unescaped_single_quote_in_double_quoted_string) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::unescaped_single_quote_in_double_quoted};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "it's"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader,
    context_get_keyword_escaped_double_quote_in_single_quoted_string) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::escaped_double_quote_in_single_quoted};
  // The escape is returned as written, not unescaped.
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "a\\\"b"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader,
    context_get_keyword_escaped_single_quote_in_double_quoted_string) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::escaped_single_quote_in_double_quoted};
  // The escape is returned as written, not unescaped.
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::string, "it\\'s"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_nested_arrays) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::nested_arrays};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "2"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "3"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_nested_objects) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::nested_objects};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "a"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "b"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "c"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::null_value, "null"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_mixed_types) {
  ubench::sjson::detail::json_reader_context context{valid_inputs::mixed_types};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "two"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::bool_true, "true"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::bool_false, "false"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::null_value, "null"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "k"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_array_of_objects_with_arrays) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::array_of_objects_with_arrays};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "a"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::integer, "1"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "b"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "c"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::bool_true, "true"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_object_of_arrays_with_objects) {
  ubench::sjson::detail::json_reader_context context{
      valid_inputs::object_of_arrays_with_objects};
  const token_list expected{
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "x"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::string, "y"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::null_value, "null"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::integer, "2"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::object_open, "{"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::comma, ","},
      {ubench::sjson::detail::json_string_keyword::string, "z"},
      {ubench::sjson::detail::json_string_keyword::object_colon, ":"},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_open, "["},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::array_close, "]"},
      {ubench::sjson::detail::json_string_keyword::object_close, "}"},
  };
  expect_keywords(context, expected);
}

TEST(sjson_reader, context_get_keyword_out_of_range_integers) {
  for (const auto input : invalid_inputs::out_of_range_integers) {
    SCOPED_TRACE(std::string{input});
    ubench::sjson::detail::json_reader_context context{input};
    // Input is "[<integer>]"; the integer is only a keyword here.
    std::string integer{input.substr(1, input.size() - 2)};
    const token_list expected{
        {ubench::sjson::detail::json_string_keyword::array_open, "["},
        {ubench::sjson::detail::json_string_keyword::integer, integer},
        {ubench::sjson::detail::json_string_keyword::array_close, "]"},
    };
    expect_keywords(context, expected);
  }
}

TEST(sjson_reader, context_get_keyword_invalid_utf8_string_is_token) {
  for (const auto input : invalid_inputs::invalid_utf8_strings) {
    SCOPED_TRACE(std::string{input});
    ubench::sjson::detail::json_reader_context context{input};
    // Tokens are: "[", the string, "]".
    ASSERT_OPTIONAL(context.get_keyword());
    auto str = context.get_keyword();
    ASSERT_OPTIONAL(str);
    EXPECT_EQ(
        str->keyword(), ubench::sjson::detail::json_string_keyword::string);
    EXPECT_FALSE(context.has_error());
  }
}

TEST(sjson_reader, context_get_keyword_invalid_sequence_returns_nullopt) {
  for (const auto input : invalid_inputs::invalid_sequences) {
    SCOPED_TRACE(std::string{input});
    ubench::sjson::detail::json_reader_context context{input};
    // Valid tokens may precede the invalid sequence; it must end the stream.
    while (context.get_keyword()) {
    }
    EXPECT_TRUE(context.has_error());
  }
}

TEST(sjson_reader, context_get_keyword_returns_tokens_before_invalid_sequence) {
  ubench::sjson::detail::json_reader_context context{
      invalid_inputs::invalid_after_token};
  auto token = context.get_keyword();
  ASSERT_OPTIONAL(token);
  EXPECT_EQ(
      token->keyword(), ubench::sjson::detail::json_string_keyword::array_open);
  ASSERT_OPTIONAL_HAS_ERROR(context.get_keyword());
  EXPECT_TRUE(context.has_error());
}

TEST(sjson_reader, to_token_array_open_returns_json_array) {
  ubench::sjson::detail::json_reader_token token{
      ubench::sjson::detail::json_string_keyword::array_open, "["};
  // We create a dummy context to create the iterator. For this test, we don't
  // iterate, so the context is not used.
  auto context = std::make_shared<ubench::sjson::detail::json_reader_context>(
      std::string_view{});
  auto actual = ubench::sjson::detail::to_token(token, context);
  ASSERT_OPTIONAL(actual);
  EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_array>(*actual));
}

TEST(sjson_reader, to_token_object_open_returns_json_object) {
  ubench::sjson::detail::json_reader_token token{
      ubench::sjson::detail::json_string_keyword::object_open, "{"};
  // We create a dummy context to create the iterator. For this test, we don't
  // iterate, so the context is not used.
  auto context = std::make_shared<ubench::sjson::detail::json_reader_context>(
      std::string_view{});
  const auto actual = ubench::sjson::detail::to_token(token, context);
  ASSERT_OPTIONAL(actual);
  EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_object>(*actual));
}

TEST(sjson_reader, to_token_integer_returns_json_int) {
  const std::vector<std::pair<std::string_view, std::int64_t>> cases{
      {"0", 0},
      {"123", 123},
      {"-45", -45},
      {"+7", 7},
      {"9223372036854775807", INT64_MAX},
      {"-9223372036854775808", INT64_MIN},
  };
  for (const auto& [text, expected] : cases) {
    SCOPED_TRACE(std::string{text});
    ubench::sjson::detail::json_reader_token token{
        ubench::sjson::detail::json_string_keyword::integer, text};
    auto actual = ubench::sjson::detail::to_token(token);
    ASSERT_OPTIONAL(actual);
    ASSERT_TRUE(std::holds_alternative<ubench::sjson::json_int>(*actual));
    EXPECT_EQ(
        static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*actual)),
        expected);
  }
}

TEST(sjson_reader, to_token_integer_out_of_range_returns_nullopt) {
  for (const std::string_view text : {"9223372036854775808",
           "-9223372036854775809", "99999999999999999999999"}) {
    SCOPED_TRACE(std::string{text});
    ubench::sjson::detail::json_reader_token token{
        ubench::sjson::detail::json_string_keyword::integer, text};
    EXPECT_FALSE(ubench::sjson::detail::to_token(token));
  }
}

TEST(sjson_reader, to_token_bool_true_returns_json_bool) {
  ubench::sjson::detail::json_reader_token token{
      ubench::sjson::detail::json_string_keyword::bool_true, "true"};
  auto actual = ubench::sjson::detail::to_token(token);
  ASSERT_OPTIONAL(actual);
  ASSERT_TRUE(std::holds_alternative<ubench::sjson::json_bool>(*actual));
  EXPECT_TRUE(static_cast<bool>(std::get<ubench::sjson::json_bool>(*actual)));
}

TEST(sjson_reader, to_token_bool_false_returns_json_bool) {
  ubench::sjson::detail::json_reader_token token{
      ubench::sjson::detail::json_string_keyword::bool_false, "false"};
  auto actual = ubench::sjson::detail::to_token(token);
  ASSERT_OPTIONAL(actual);
  ASSERT_TRUE(std::holds_alternative<ubench::sjson::json_bool>(*actual));
  EXPECT_FALSE(static_cast<bool>(std::get<ubench::sjson::json_bool>(*actual)));
}

TEST(sjson_reader, to_token_null_returns_json_null) {
  ubench::sjson::detail::json_reader_token token{
      ubench::sjson::detail::json_string_keyword::null_value, "null"};
  auto actual = ubench::sjson::detail::to_token(token);
  ASSERT_OPTIONAL(actual);
  EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_null>(*actual));
}

TEST(sjson_reader, to_token_structural_keywords_return_nullopt) {
  for (const auto keyword :
      {ubench::sjson::detail::json_string_keyword::array_close,
          ubench::sjson::detail::json_string_keyword::object_close,
          ubench::sjson::detail::json_string_keyword::object_colon,
          ubench::sjson::detail::json_string_keyword::comma}) {
    ubench::sjson::detail::json_reader_token token{keyword, ","};
    EXPECT_FALSE(ubench::sjson::detail::to_token(token));
  }
}

TEST(sjson_reader, to_token_string_unescaped) {
  // Each input is a quoted JSON string; the token value is the content between
  // the quotes, so it points into the input buffer.
  const std::vector<std::string_view> inputs{
      R"("€")",
      R"("Straße")",
      R"("Groß-Bärenhöhle")",
      R"("€uro ß")",
      R"("Ærøskøbing")",
      R"("Łódź")",
      R"("Zażółć gęślą jaźń")",
      R"("Ñandú")",
      R"("Привет")",
      R"("Ελληνικά")",
  };
  for (const auto input : inputs) {
    SCOPED_TRACE(std::string{input});
    const std::string_view content{input.substr(1, input.size() - 2)};
    ubench::sjson::detail::json_reader_token token{
        ubench::sjson::detail::json_string_keyword::string, content};
    const auto actual = ubench::sjson::detail::to_token(token);
    ASSERT_OPTIONAL(actual);
    ASSERT_TRUE(std::holds_alternative<ubench::sjson::json_string>(*actual));
    const std::string_view view{std::get<ubench::sjson::json_string>(*actual)};
    EXPECT_EQ(view, content);
    EXPECT_EQ(view.data(), content.data());
  }
}

TEST(sjson_reader, to_token_string_escaped_simple_characters) {
  const std::vector<std::pair<std::string_view, std::string>> cases{
      {R"(\\)", "\\"},
      {R"(\")", "\""},
      {R"(\')", "'"},
      {R"(\/)", "/"},
      {R"(\b)", "\b"},
      {R"(\f)", "\f"},
      {R"(\n)", "\n"},
      {R"(\r)", "\r"},
      {R"(\t)", "\t"},
      {R"(\v)", "\v"},
      {R"(a\/b)", "a/b"},
      {R"(\()", "("},
      {R"(\))", ")"},
      {R"(\[)", "["},
      {R"(\a)", "a"},
      {R"(C:\\temp\\\"x\")", R"(C:\temp\"x")"},
  };
  for (const auto& [content, expected] : cases) {
    SCOPED_TRACE(std::string{content});
    ubench::sjson::detail::json_reader_token token{
        ubench::sjson::detail::json_string_keyword::string, content};
    const auto actual = ubench::sjson::detail::to_token(token);
    ASSERT_OPTIONAL(actual);
    ASSERT_TRUE(std::holds_alternative<ubench::sjson::json_string>(*actual));
    const std::string_view view{std::get<ubench::sjson::json_string>(*actual)};
    EXPECT_EQ(view, expected);
    EXPECT_NE(view.data(), content.data());
  }
}

TEST(sjson_reader, to_token_string_escaped_unicode_bmp) {
  const std::vector<std::pair<std::string_view, std::string>> cases{
      {R"(\u0041)", "A"},
      {R"(\u0000)", std::string("\0", 1)},
      {R"(\u007F)", "\x7F"},
      {R"(\u0080)", "\xC2\x80"},
      {R"(\u00e9)", "é"},
      {R"(\u00DF)", "ß"},
      {R"(\u07FF)", "\xDF\xBF"},
      {R"(\u0800)", "\xE0\xA0\x80"},
      {R"(\u20AC)", "€"},
      {R"(\u20ac)", "€"},
      {R"(\uFFFF)", "\xEF\xBF\xBF"},
      {R"(a\u00e9\u20AC\u0041z)", "aé€Az"},
      {R"(\u20AC\/\\\")", "€/\\\""},
  };
  for (const auto& [content, expected] : cases) {
    SCOPED_TRACE(std::string{content});
    ubench::sjson::detail::json_reader_token token{
        ubench::sjson::detail::json_string_keyword::string, content};
    const auto actual = ubench::sjson::detail::to_token(token);
    ASSERT_OPTIONAL(actual);
    ASSERT_TRUE(std::holds_alternative<ubench::sjson::json_string>(*actual));
    const std::string_view view{std::get<ubench::sjson::json_string>(*actual)};
    EXPECT_EQ(view, expected);
    EXPECT_NE(view.data(), content.data());
  }
}

TEST(sjson_reader, to_token_string_escaped_surrogate_pairs) {
  const std::vector<std::pair<std::string_view, std::string>> cases{
      // U+10000, the first supplementary code point.
      {R"(\uD800\uDC00)", "\xF0\x90\x80\x80"},
      // U+1F600 (grinning face).
      {R"(\uD83D\uDE00)", "😀"},
      // U+1D11E (musical symbol G clef).
      {R"(\uD834\uDD1E)", "\xF0\x9D\x84\x9E"},
      // U+10FFFF, the last code point.
      {R"(\uDBFF\uDFFF)", "\xF4\x8F\xBF\xBF"},
      {R"(\uD83D\uDE00\u20AC)", "😀€"},
      {R"(x\uD83D\uDE00y)", "x😀y"},
      {R"(\uD83D\uDE00\uD83D\uDE00)", "😀😀"},
  };
  for (const auto& [content, expected] : cases) {
    SCOPED_TRACE(std::string{content});
    ubench::sjson::detail::json_reader_token token{
        ubench::sjson::detail::json_string_keyword::string, content};
    const auto actual = ubench::sjson::detail::to_token(token);
    ASSERT_OPTIONAL(actual);
    ASSERT_TRUE(std::holds_alternative<ubench::sjson::json_string>(*actual));
    const std::string_view view{std::get<ubench::sjson::json_string>(*actual)};
    EXPECT_EQ(view, expected);
    EXPECT_NE(view.data(), content.data());
  }
}

namespace {

/// @brief One get_token() call: the depth after it, and what it returned.
///
/// The label is "end" when the call returned std::nullopt because a block was
/// closed.
struct token_event {
  std::size_t depth;
  std::string label;

  auto operator==(const token_event& other) const -> bool {
    return depth == other.depth && label == other.label;
  }
};

auto describe_token(const ubench::sjson::json_token& token) -> std::string {
  return std::visit(
      [](const auto& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ubench::sjson::json_int>)
          return std::to_string(static_cast<std::int64_t>(arg));
        else if constexpr (std::is_same_v<T, ubench::sjson::json_bool>)
          return static_cast<bool>(arg) ? "true" : "false";
        else if constexpr (std::is_same_v<T, ubench::sjson::json_null>)
          return "null";
        else if constexpr (std::is_same_v<T, ubench::sjson::json_string>)
          return "\"" + std::string{static_cast<std::string_view>(arg)} + "\"";
        else if constexpr (std::is_same_v<T, ubench::sjson::json_array>)
          return "[";
        else
          return "{";
      },
      token);
}

/// @brief Call get_token() once and describe the result.
///
/// @return std::nullopt if the call neither returned a token nor closed a
/// block, which means the input ended or an error occurred.
auto next_event(ubench::sjson::detail::json_reader_context& context)
    -> std::optional<token_event> {
  auto depth_before = context.depth();
  auto member = context.get_token();
  if (member) {
    std::string label;
    if (auto* key = std::get_if<ubench::sjson::json_key>(&*member))
      label = std::string{key->key()} + "=" + describe_token(key->value());
    else
      label = describe_token(std::get<ubench::sjson::json_token>(*member));
    return token_event{context.depth(), label};
  }
  if (context.depth() < depth_before)
    return token_event{context.depth(), "end"};
  return std::nullopt;
}

/// @brief Every get_token() result of json, from a fresh reader.
auto full_trace(std::string_view json) -> std::vector<token_event> {
  auto owner =
      std::make_shared<ubench::sjson::detail::json_reader_context>(json);
  auto& context = *owner;
  std::vector<token_event> events;
  while (auto event = next_event(context)) events.push_back(*event);
  return events;
}

/// @brief Check skip_block() after every get_token() call on json.
///
/// After each event of the full trace, a fresh reader is replayed up to that
/// event and skip_block() is called. It must consume every remaining token of
/// the block at the current depth, leaving the reader at the enclosing depth
/// without error. The remaining events must equal the full trace after that
/// block's end.
auto expect_skip_block_after_every_token(std::string_view json) -> void {
  const auto trace = full_trace(json);
  ASSERT_FALSE(trace.empty());

  for (std::size_t event{}; event < trace.size(); ++event) {
    SCOPED_TRACE(event);
    auto depth = trace[event].depth;

    // At depth zero there is no block to skip. Otherwise the block is closed by
    // the first later end event at the enclosing depth.
    std::size_t close = event;
    if (depth > 0) {
      close = event + 1;
      while (close < trace.size() &&
             !(trace[close].label == "end" && trace[close].depth == depth - 1))
        ++close;
      ASSERT_LT(close, trace.size());
    }
    const auto expected_depth = depth > 0 ? depth - 1 : 0;

    auto owner =
        std::make_shared<ubench::sjson::detail::json_reader_context>(json);
    auto& context = *owner;
    for (std::size_t i{}; i <= event; ++i) {
      const auto actual = next_event(context);
      ASSERT_OPTIONAL_EQ(actual, trace[i]);
    }

    context.skip_block();
    EXPECT_FALSE(context.has_error());
    EXPECT_EQ(context.depth(), expected_depth);

    for (std::size_t i = close + 1; i < trace.size(); ++i) {
      const auto actual = next_event(context);
      ASSERT_OPTIONAL_EQ(actual, trace[i]);
    }
    ASSERT_OPTIONAL_HAS_ERROR(next_event(context));
    EXPECT_FALSE(context.has_error());
  }
}

}  // namespace

TEST(sjson_reader, context_skip_block_after_every_token) {
  expect_skip_block_after_every_token(valid_inputs::every_token);
}

TEST(sjson_reader, context_skip_block_unterminated_block_returns_error) {
  auto owner =
      std::make_shared<ubench::sjson::detail::json_reader_context>("[1,[2");
  auto& context = *owner;
  auto open = next_event(context);
  ASSERT_OPTIONAL(open);
  context.skip_block();
  EXPECT_TRUE(context.has_error());
}

namespace {

/// @brief Read every token and check that the input is reported as an error.
auto expect_get_token_error(std::string_view json) -> void {
  auto owner =
      std::make_shared<ubench::sjson::detail::json_reader_context>(json);
  auto& context = *owner;
  while (next_event(context)) {
  }
  EXPECT_TRUE(context.has_error());
}

}  // namespace

/// @brief Run expect_get_token_error() and report the input's variable name on
/// failure.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_GET_TOKEN_ERROR(json) \
  do {                               \
    SCOPED_TRACE(#json);             \
    expect_get_token_error(json);    \
  } while (false)

// Container roots with trailing tokens (root_two_arrays, root_two_objects,
// root_array_object, root_array_int, root_object_string, root_extra_close) are
// not covered yet. The root container closes the input, so they are not
// rejected.
TEST(sjson_reader, context_get_token_invalid_grammar_root_returns_error) {
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::root_int);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::root_string);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::root_true);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::root_false);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::root_null);
}

TEST(sjson_reader, context_get_token_invalid_grammar_array_returns_error) {
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_no_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_double_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_leading_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_only_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_array_no_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_object_no_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_unclosed);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_unclosed_nested);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_extra_close);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_mismatch_close);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_member);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_colon);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_nested_member);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::arr_object_mismatch);
}

TEST(sjson_reader, context_get_token_invalid_grammar_object_returns_error) {
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_no_colon);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_no_value);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_no_key);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_colon_only);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_int_key);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_null_key);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_bool_key);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_array_key);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_value_no_key);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_key_no_colon);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_no_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_double_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_leading_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_double_colon);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_nested_no_colon);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_unclosed);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_unclosed_nested);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_mismatch_close);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::obj_extra_close);
}

TEST(sjson_reader, context_get_token_invalid_grammar_mixed_returns_error) {
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::mix_arr_in_obj_unclosed);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::mix_obj_in_arr_no_comma);
  EXPECT_GET_TOKEN_ERROR(invalid_grammar::mix_arr_then_member);
}

TEST(sjson_reader, context_get_token_out_of_range_integer_returns_error) {
  for (const auto input : invalid_inputs::out_of_range_integers) {
    SCOPED_TRACE(std::string{input});
    auto owner =
        std::make_shared<ubench::sjson::detail::json_reader_context>(input);
    auto& context = *owner;
    while (context.get_token()) {
    }
    EXPECT_TRUE(context.has_error());
  }
}

TEST(sjson_reader, context_get_token_invalid_utf8_string_returns_error) {
  for (const auto input : invalid_inputs::invalid_utf8_strings) {
    SCOPED_TRACE(std::string{input});
    auto owner =
        std::make_shared<ubench::sjson::detail::json_reader_context>(input);
    auto& context = *owner;
    while (context.get_token()) {
    }
    EXPECT_TRUE(context.has_error());
  }
}

TEST(sjson_reader, mapped_copy_outlives_original) {
  const auto path = test_output_path("mapped_copy_outlives.json");
  {
    std::ofstream out{path, std::ios::binary};
    out << "[1]";
  }
  std::optional<ubench::sjson::detail::json_reader_context> copy;
  {
    auto file = ubench::sjson::detail::mapped_file::open(path);
    ASSERT_TRUE(file.has_value());
    ubench::sjson::detail::json_reader_context original{
        std::make_shared<ubench::sjson::detail::mapped_file>(std::move(*file))};
    copy.emplace(original);
  }
  ASSERT_TRUE(copy.has_value());
  auto token = copy->get_keyword();
  ASSERT_OPTIONAL(token);
  EXPECT_EQ(
      token->keyword(), ubench::sjson::detail::json_string_keyword::array_open);
}

TEST(sjson_reader, mapped_file_open_missing_returns_errno) {
  const auto path = std::filesystem::temp_directory_path() /
                    "sjson_mapped_file_open_missing_does_not_exist.json";
  const auto file = ubench::sjson::detail::mapped_file::open(path);
  ASSERT_FALSE(file.has_value());
  EXPECT_EQ(file.error(), ENOENT);
}

TEST(sjson_reader, mapped_file_move_assign_replaces_mapping) {
  const auto first_path =
      test_output_path("mapped_file_move_assign_first.json");
  const auto second_path =
      test_output_path("mapped_file_move_assign_second.json");
  {
    std::ofstream out{first_path, std::ios::binary};
    out << "[1]";
  }
  {
    std::ofstream out{second_path, std::ios::binary};
    out << "{}";
  }
  auto first = ubench::sjson::detail::mapped_file::open(first_path);
  auto second = ubench::sjson::detail::mapped_file::open(second_path);
  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(first->view(), "[1]");
  *first = std::move(*second);
  EXPECT_EQ(first->view(), "{}");
}

TEST(sjson_reader, mapped_file_open_empty_has_empty_view) {
  const auto path = test_output_path("mapped_file_open_empty.json");
  { std::ofstream out{path, std::ios::binary}; }
  auto file = ubench::sjson::detail::mapped_file::open(path);
  ASSERT_TRUE(file.has_value());
  EXPECT_TRUE(file->view().empty());
}
