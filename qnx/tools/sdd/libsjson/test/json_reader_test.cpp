#include "sjson/json_reader.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include <gtest/gtest.h>

#include "rtest/rtest.h"
#include "json_reader_context_inputs.h"
#include "test_output.h"

namespace valid_inputs = ubench::sjson::detail::valid_inputs;
namespace invalid_grammar = ubench::sjson::detail::invalid_grammar;

// json_int and json_bool must remain usable in constant expressions.
static_assert(ubench::sjson::json_int{42} == 42);
static_assert(ubench::sjson::json_bool{true} == true);
static_assert(ubench::sjson::json_bool{false} == false);
static_assert(
    std::is_nothrow_default_constructible_v<ubench::sjson::json_null>);

// The reader and every object it hands out must be movable.
static_assert(std::is_nothrow_move_constructible_v<ubench::sjson::json_reader>);
static_assert(std::is_move_assignable_v<ubench::sjson::json_reader>);
static_assert(std::is_nothrow_move_constructible_v<ubench::sjson::json_array>);
static_assert(std::is_move_assignable_v<ubench::sjson::json_array>);
static_assert(std::is_nothrow_move_constructible_v<ubench::sjson::json_object>);
static_assert(std::is_move_assignable_v<ubench::sjson::json_object>);
static_assert(std::is_nothrow_move_constructible_v<ubench::sjson::json_key>);
static_assert(std::is_move_assignable_v<ubench::sjson::json_key>);
static_assert(std::is_nothrow_move_constructible_v<ubench::sjson::json_token>);
static_assert(std::is_move_assignable_v<ubench::sjson::json_token>);
static_assert(
    std::is_nothrow_move_constructible_v<ubench::sjson::json_array_iterator>);
static_assert(std::is_move_assignable_v<ubench::sjson::json_array_iterator>);
static_assert(
    std::is_nothrow_move_constructible_v<ubench::sjson::json_object_iterator>);
static_assert(std::is_move_assignable_v<ubench::sjson::json_object_iterator>);

TEST(sjson_reader, json_int_value) {
  ubench::sjson::json_int value{42};
  EXPECT_EQ(static_cast<std::int64_t>(value), 42);
}

TEST(sjson_reader, json_int_negative) {
  ubench::sjson::json_int value{-7};
  EXPECT_EQ(static_cast<std::int64_t>(value), -7);
}

TEST(sjson_reader, json_int_limits) {
  constexpr auto max = std::numeric_limits<std::int64_t>::max();
  constexpr auto min = std::numeric_limits<std::int64_t>::min();
  EXPECT_EQ(static_cast<std::int64_t>(ubench::sjson::json_int{max}), max);
  EXPECT_EQ(static_cast<std::int64_t>(ubench::sjson::json_int{min}), min);
}

TEST(sjson_reader, json_int_implicit_conversion) {
  ubench::sjson::json_int value{123};
  std::int64_t converted = value;
  EXPECT_EQ(converted, 123);
}

TEST(sjson_reader, json_int_is_copyable) {
  ubench::sjson::json_int original{99};
  ubench::sjson::json_int copy{original};
  EXPECT_EQ(static_cast<std::int64_t>(copy), 99);
  static_assert(std::is_trivially_copyable_v<ubench::sjson::json_int>);
}

TEST(sjson_reader, json_bool_true) {
  ubench::sjson::json_bool value{true};
  EXPECT_TRUE(static_cast<bool>(value));
}

TEST(sjson_reader, json_bool_false) {
  ubench::sjson::json_bool value{false};
  EXPECT_FALSE(static_cast<bool>(value));
}

TEST(sjson_reader, json_bool_implicit_conversion) {
  ubench::sjson::json_bool value{true};
  bool converted = value;
  EXPECT_TRUE(converted);
}

TEST(sjson_reader, json_bool_is_copyable) {
  ubench::sjson::json_bool original{false};
  ubench::sjson::json_bool copy{original};
  EXPECT_FALSE(static_cast<bool>(copy));
  static_assert(std::is_trivially_copyable_v<ubench::sjson::json_bool>);
}

TEST(sjson_reader, json_null_default_construct) {
  constexpr ubench::sjson::json_null value{};
  static_cast<void>(value);
  SUCCEED();
}

TEST(sjson_reader, json_null_is_copyable) {
  ubench::sjson::json_null original{};
  ubench::sjson::json_null copy{original};
  ubench::sjson::json_null assigned{};
  assigned = copy;
  static_cast<void>(assigned);
  static_assert(std::is_trivially_copyable_v<ubench::sjson::json_null>);
  SUCCEED();
}

TEST(sjson_reader, json_null_is_nothrow_default_constructible) {
  EXPECT_TRUE(
      std::is_nothrow_default_constructible_v<ubench::sjson::json_null>);
}

namespace {

constexpr std::string_view string_long{"a string long enough to avoid SSO"};
constexpr std::string_view string_short{"short"};

auto as_view(const ubench::sjson::json_string& value) -> std::string_view {
  return value;
}

auto as_int(const ubench::sjson::json_token& value) -> std::int64_t {
  return static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(value));
}

}  // namespace

TEST(sjson_reader, json_string_view_refers_to_source) {
  ubench::sjson::json_string value{string_long};
  EXPECT_EQ(as_view(value), string_long);
  EXPECT_EQ(as_view(value).data(), string_long.data());
}

TEST(sjson_reader, json_string_string_is_owned) {
  std::string source{string_long};
  ubench::sjson::json_string value{source};
  EXPECT_EQ(as_view(value), source);
  EXPECT_NE(as_view(value).data(), source.data());
}

TEST(sjson_reader, json_string_owned_outlives_source) {
  std::optional<ubench::sjson::json_string> value;
  {
    std::string source{string_long};
    value.emplace(std::move(source));
  }
  EXPECT_EQ(as_view(*value), string_long);
}

TEST(sjson_reader, json_string_copy_of_view_shares_source) {
  ubench::sjson::json_string original{string_long};
  ubench::sjson::json_string copy{original};
  EXPECT_EQ(as_view(copy).data(), string_long.data());
}

TEST(sjson_reader, json_string_copy_of_owned_is_deep) {
  std::optional<ubench::sjson::json_string> copy;
  {
    ubench::sjson::json_string original{std::string{string_long}};
    copy.emplace(original);
    EXPECT_EQ(as_view(*copy), string_long);
    EXPECT_NE(as_view(*copy).data(), as_view(original).data());
  }
  EXPECT_EQ(as_view(*copy), string_long);
}

TEST(sjson_reader, json_string_move_of_owned_transfers_ownership) {
  std::optional<ubench::sjson::json_string> moved;
  {
    ubench::sjson::json_string original{std::string{string_long}};
    moved.emplace(std::move(original));
    EXPECT_EQ(as_view(*moved), string_long);

    // The moved-from object must still be assignable.
    original = ubench::sjson::json_string{std::string_view{"reset"}};
    EXPECT_EQ(as_view(original), std::string_view{"reset"});
  }
  EXPECT_EQ(as_view(*moved), string_long);
}

TEST(sjson_reader, json_string_move_of_view_keeps_source) {
  ubench::sjson::json_string original{string_long};
  ubench::sjson::json_string moved{std::move(original)};
  EXPECT_EQ(as_view(moved).data(), string_long.data());

  // The moved-from object must still be a usable view.
  original = ubench::sjson::json_string{string_short};
  EXPECT_EQ(as_view(original), string_short);
  EXPECT_EQ(as_view(moved).data(), string_long.data());
}

TEST(sjson_reader, json_string_copy_assign_view_over_owned) {
  ubench::sjson::json_string owned{std::string{string_long}};
  ubench::sjson::json_string view{string_short};
  owned = view;
  EXPECT_EQ(as_view(owned).data(), string_short.data());
  EXPECT_EQ(as_view(view).data(), string_short.data());
}

TEST(sjson_reader, json_string_copy_assign_owned_over_view) {
  ubench::sjson::json_string view{string_short};
  ubench::sjson::json_string owned{std::string{string_long}};
  view = owned;
  EXPECT_EQ(as_view(view), string_long);
  EXPECT_NE(as_view(view).data(), as_view(owned).data());
}

TEST(sjson_reader, json_string_move_assign_owned_over_view) {
  ubench::sjson::json_string target{string_short};
  ubench::sjson::json_string source{std::string{string_long}};
  target = std::move(source);
  EXPECT_EQ(as_view(target), string_long);

  // The moved-from object must still be assignable, and must not affect target.
  source = ubench::sjson::json_string{string_short};
  EXPECT_EQ(as_view(source), string_short);
  EXPECT_EQ(as_view(target), string_long);
}

TEST(sjson_reader, json_string_move_assign_view_over_owned) {
  ubench::sjson::json_string target{std::string{string_long}};
  ubench::sjson::json_string source{string_short};
  target = std::move(source);
  EXPECT_EQ(as_view(target).data(), string_short.data());

  // Swap-based move assignment leaves the old target state in the source.
  source = ubench::sjson::json_string{std::string_view{"after"}};
  EXPECT_EQ(as_view(source), std::string_view{"after"});
  EXPECT_EQ(as_view(target).data(), string_short.data());
}

TEST(sjson_reader, json_string_self_copy_assignment) {
  ubench::sjson::json_string owned{std::string{string_long}};
  ubench::sjson::json_string& owned_alias = owned;
  owned = owned_alias;
  EXPECT_EQ(as_view(owned), string_long);

  ubench::sjson::json_string view{string_short};
  ubench::sjson::json_string& view_alias = view;
  view = view_alias;
  EXPECT_EQ(as_view(view).data(), string_short.data());
}

TEST(sjson_reader, json_string_self_move_assignment) {
  ubench::sjson::json_string owned{std::string{string_long}};
  ubench::sjson::json_string& owned_alias = owned;
  owned = std::move(owned_alias);
  EXPECT_EQ(as_view(owned), string_long);

  ubench::sjson::json_string view{string_short};
  ubench::sjson::json_string& view_alias = view;
  view = std::move(view_alias);
  EXPECT_EQ(as_view(view).data(), string_short.data());
}

namespace {

/// @brief Check that root() of valid input returns an array or object.
auto expect_container_root(std::string_view json) -> void {
  ubench::sjson::json_reader reader{json};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_array>(*root) ||
              std::holds_alternative<ubench::sjson::json_object>(*root));
  EXPECT_FALSE(reader.has_error());
}

/// @brief Check that root() of input without a root returns std::nullopt.
auto expect_no_container_root(std::string_view json) -> void {
  ubench::sjson::json_reader reader{json};
  ASSERT_OPTIONAL_HAS_ERROR(reader.root());
  EXPECT_FALSE(reader.has_error());
}

}  // namespace

/// @brief Run expect_container_root() and report the input's variable name on
/// failure.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_CONTAINER_ROOT(json) \
  do {                              \
    SCOPED_TRACE(#json);            \
    expect_container_root(json);    \
  } while (false)

TEST(sjson_reader, root_empty_input_returns_nullopt) {
  expect_no_container_root(valid_inputs::empty);
}

TEST(sjson_reader, root_only_comment_returns_nullopt) {
  expect_no_container_root(valid_inputs::only_comment);
}

TEST(sjson_reader, root_array_inputs) {
  EXPECT_CONTAINER_ROOT(valid_inputs::comment_at_start);
  EXPECT_CONTAINER_ROOT(valid_inputs::comment_in_middle);
  EXPECT_CONTAINER_ROOT(valid_inputs::comment_after_line);
  EXPECT_CONTAINER_ROOT(valid_inputs::newlines_cr);
  EXPECT_CONTAINER_ROOT(valid_inputs::newlines_crlf);
  EXPECT_CONTAINER_ROOT(valid_inputs::newlines_lf);
  EXPECT_CONTAINER_ROOT(valid_inputs::whitespace);
  EXPECT_CONTAINER_ROOT(valid_inputs::keywords);
  EXPECT_CONTAINER_ROOT(valid_inputs::integers);
  EXPECT_CONTAINER_ROOT(valid_inputs::strings);
  EXPECT_CONTAINER_ROOT(valid_inputs::plus_prefixed_integers);
  EXPECT_CONTAINER_ROOT(valid_inputs::utf8_strings);
  EXPECT_CONTAINER_ROOT(valid_inputs::array_of_integers_trailing_comma);
  EXPECT_CONTAINER_ROOT(valid_inputs::escaped_quotes_in_string);
  EXPECT_CONTAINER_ROOT(valid_inputs::single_quoted_strings);
  EXPECT_CONTAINER_ROOT(valid_inputs::escaped_backslash_in_string);
  EXPECT_CONTAINER_ROOT(valid_inputs::escaped_slash_in_string);
  EXPECT_CONTAINER_ROOT(valid_inputs::unescaped_double_quote_in_single_quoted);
  EXPECT_CONTAINER_ROOT(valid_inputs::unescaped_single_quote_in_double_quoted);
  EXPECT_CONTAINER_ROOT(valid_inputs::escaped_double_quote_in_single_quoted);
  EXPECT_CONTAINER_ROOT(valid_inputs::escaped_single_quote_in_double_quoted);
  EXPECT_CONTAINER_ROOT(valid_inputs::nested_arrays);
  EXPECT_CONTAINER_ROOT(valid_inputs::mixed_types);
}

TEST(sjson_reader, root_object_inputs) {
  EXPECT_CONTAINER_ROOT(valid_inputs::punctuation);
  EXPECT_CONTAINER_ROOT(valid_inputs::nested_objects);
  EXPECT_CONTAINER_ROOT(valid_inputs::object_of_integers_trailing_comma);
}

TEST(sjson_reader, root_array_of_objects_and_arrays_inputs) {
  EXPECT_CONTAINER_ROOT(valid_inputs::array_of_objects_with_arrays);
  EXPECT_CONTAINER_ROOT(valid_inputs::object_of_arrays_with_objects);
}

TEST(sjson_reader, read_root_twice) {
  const std::array<std::pair<std::string_view, std::string_view>, 4> inputs{{
      {"single_quoted_strings", valid_inputs::single_quoted_strings},
      {"comment_at_start", valid_inputs::comment_at_start},
      {"comment_in_middle", valid_inputs::comment_in_middle},
      {"comment_after_line", valid_inputs::comment_after_line},
  }};

  for (const auto& [name, json] : inputs) {
    SCOPED_TRACE(name);
    ubench::sjson::json_reader reader{json};
    const auto root = reader.root();
    ASSERT_OPTIONAL(root);
    EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_array>(*root) ||
                std::holds_alternative<ubench::sjson::json_object>(*root));
    EXPECT_FALSE(reader.has_error());

    EXPECT_THROW(reader.root(), ubench::sjson::json_reader_error);
  }
}

namespace {

/// @brief Serialise a token tree in a compact canonical form.
///
/// Integers are decimal, booleans and null are literal, strings are wrapped in
/// double quotes without escaping, arrays are [a,b] and objects are {"k":v}.
/// Quote style, whitespace, and a leading '+' do not appear in the output, so
/// the canonical form describes the tokens rather than the source text.
auto append_canonical(std::string& out, const ubench::sjson::json_token& token)
    -> void;

auto append_canonical(std::string& out, const ubench::sjson::json_array& array)
    -> void {
  out += '[';
  bool first = true;
  for (const auto& t : array) {
    if (!first) out += ',';
    first = false;
    append_canonical(out, t);
  }
  out += ']';
}

auto append_canonical(std::string& out, const ubench::sjson::json_object& obj)
    -> void {
  out += '{';
  bool first = true;
  for (const auto& member : obj) {
    if (!first) out += ',';
    first = false;
    out += '"';
    out += member.key();
    out += "\":";
    append_canonical(out, member.value());
  }
  out += '}';
}

auto append_canonical(std::string& out, const ubench::sjson::json_token& token)
    -> void {
  std::visit(
      [&out](const auto& arg) -> void {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ubench::sjson::json_int>)
          out += std::to_string(static_cast<std::int64_t>(arg));
        else if constexpr (std::is_same_v<T, ubench::sjson::json_bool>)
          out += static_cast<bool>(arg) ? "true" : "false";
        else if constexpr (std::is_same_v<T, ubench::sjson::json_null>)
          out += "null";
        else if constexpr (std::is_same_v<T, ubench::sjson::json_string>) {
          out += '"';
          out += static_cast<std::string_view>(arg);
          out += '"';
        } else
          append_canonical(out, arg);
      },
      token);
}

/// @brief Parse json and check that its canonical form equals expected.
///
/// Also checks that root() succeeds and reports no error.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto expect_canonical(std::string_view json, std::string_view expected)
    -> void {
  ubench::sjson::json_reader reader{json};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  EXPECT_FALSE(reader.has_error());

  std::string actual;
  append_canonical(actual, *root);
  EXPECT_EQ(actual, expected);
}

}  // namespace

/// @brief Run expect_canonical() and report the input's variable name on
/// failure.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_CANONICAL(json, expected) \
  do {                                   \
    SCOPED_TRACE(#json);                 \
    expect_canonical(json, expected);    \
  } while (false)

TEST(sjson_reader, iterate_every_token) {
  EXPECT_CANONICAL(valid_inputs::every_token,
      R"([{"a":[1,"two",true,null,{"k":[]},[3]]},)"
      R"({"b":[false,[],{},"s",-4]},)"
      R"([{"c":null,"d":[4],"e":{"f":"g"},"h":false,"i":7},)"
      R"({"j":true,"k":"l","m":[null]}]])");
}

namespace {

auto walk_token(const ubench::sjson::json_token& token) -> void;

/// @brief Read every token of an array through its iterator.
auto walk_array(const ubench::sjson::json_array& array) -> void {
  for (const auto& t : array) walk_token(t);
}

/// @brief Read every member of an object through its iterator.
auto walk_object(const ubench::sjson::json_object& obj) -> void {
  for (const auto& member : obj) walk_token(member.value());
}

auto walk_token(const ubench::sjson::json_token& token) -> void {
  std::visit(
      []([[maybe_unused]] const auto& arg) -> void {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ubench::sjson::json_array>) {
          walk_array(arg);
        } else if constexpr (std::is_same_v<T, ubench::sjson::json_object>) {
          walk_object(arg);
        }
      },
      token);
}

/// @brief Parse json through the iterators and check that an error is set.
auto expect_iterate_error(std::string_view json) -> void {
  ubench::sjson::json_reader reader{json};
  const auto root = reader.root();
  if (root) walk_token(*root);
  EXPECT_TRUE(reader.has_error());
}

}  // namespace

/// @brief Run expect_iterate_error() and report the input's variable name on
/// failure.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_ITERATE_ERROR(json) \
  do {                             \
    SCOPED_TRACE(#json);           \
    expect_iterate_error(json);    \
  } while (false)

// Container roots with trailing tokens (root_two_arrays, root_two_objects,
// root_array_object, root_array_int, root_object_string, root_extra_close) are
// not covered yet, as in context_get_token_invalid_grammar_root_returns_error.
TEST(sjson_reader, iterate_invalid_grammar_root_returns_error) {
  EXPECT_ITERATE_ERROR(invalid_grammar::root_int);
  EXPECT_ITERATE_ERROR(invalid_grammar::root_string);
  EXPECT_ITERATE_ERROR(invalid_grammar::root_true);
  EXPECT_ITERATE_ERROR(invalid_grammar::root_false);
  EXPECT_ITERATE_ERROR(invalid_grammar::root_null);
}

TEST(sjson_reader, iterate_invalid_grammar_array_returns_error) {
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_no_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_double_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_leading_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_only_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_array_no_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_object_no_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_unclosed);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_unclosed_nested);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_extra_close);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_mismatch_close);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_member);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_colon);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_nested_member);
  EXPECT_ITERATE_ERROR(invalid_grammar::arr_object_mismatch);
}

TEST(sjson_reader, iterate_invalid_grammar_object_returns_error) {
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_no_colon);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_no_value);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_no_key);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_colon_only);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_int_key);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_null_key);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_bool_key);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_array_key);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_value_no_key);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_key_no_colon);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_no_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_double_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_leading_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_double_colon);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_nested_no_colon);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_unclosed);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_unclosed_nested);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_mismatch_close);
  EXPECT_ITERATE_ERROR(invalid_grammar::obj_extra_close);
}

TEST(sjson_reader, iterate_invalid_grammar_mixed_returns_error) {
  EXPECT_ITERATE_ERROR(invalid_grammar::mix_arr_in_obj_unclosed);
  EXPECT_ITERATE_ERROR(invalid_grammar::mix_obj_in_arr_no_comma);
  EXPECT_ITERATE_ERROR(invalid_grammar::mix_arr_then_member);
}

TEST(sjson_reader, skip_block_root_array_after_three_elements) {
  ubench::sjson::json_reader reader{valid_inputs::every_token};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& outer = std::get<ubench::sjson::json_array>(*root);
  {
    auto it = outer.begin();
    ASSERT_TRUE(it != outer.end());
    EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_object>(*it));
    ++it;
    ASSERT_TRUE(it != outer.end());
    EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_object>(*it));
    ++it;
    ASSERT_TRUE(it != outer.end());
    EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_array>(*it));
  }  // Destroying the iterator skips the rest of the root array.
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, skip_block_inner_array_mid_iteration) {
  ubench::sjson::json_reader reader{valid_inputs::every_token};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& outer = std::get<ubench::sjson::json_array>(*root);

  auto outer_it = outer.begin();
  ASSERT_TRUE(outer_it != outer.end());
  const auto& first = std::get<ubench::sjson::json_object>(*outer_it);
  auto obj_it = first.begin();
  ASSERT_TRUE(obj_it != first.end());
  EXPECT_EQ(std::string_view{(*obj_it).key()}, "a");

  const auto& values = std::get<ubench::sjson::json_array>((*obj_it).value());
  {
    auto value_it = values.begin();
    ASSERT_TRUE(value_it != values.end());
    EXPECT_EQ(
        static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*value_it)),
        1);
    ++value_it;
    ASSERT_TRUE(value_it != values.end());
    EXPECT_EQ(as_view(std::get<ubench::sjson::json_string>(*value_it)), "two");
  }  // Destroying the iterator skips true, null, {"k":[]}, [3] and ].

  ++obj_it;
  EXPECT_TRUE(obj_it == first.end());

  ++outer_it;
  ASSERT_TRUE(outer_it != outer.end());
  const auto& second = std::get<ubench::sjson::json_object>(*outer_it);
  auto second_it = second.begin();
  ASSERT_TRUE(second_it != second.end());
  EXPECT_EQ(std::string_view{(*second_it).key()}, "b");

  const auto& b_values =
      std::get<ubench::sjson::json_array>((*second_it).value());
  {
    auto b_it = b_values.begin();
    ASSERT_TRUE(b_it != b_values.end());
    EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_bool>(*b_it));
  }  // Destroying the iterator skips the rest of the "b" array.

  ++second_it;
  EXPECT_TRUE(second_it == second.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, skip_block_nested_object_mid_iteration) {
  ubench::sjson::json_reader reader{valid_inputs::every_token};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& outer = std::get<ubench::sjson::json_array>(*root);

  auto outer_it = outer.begin();
  ASSERT_TRUE(outer_it != outer.end());
  ++outer_it;
  ASSERT_TRUE(outer_it != outer.end());
  ++outer_it;
  ASSERT_TRUE(outer_it != outer.end());
  const auto& third = std::get<ubench::sjson::json_array>(*outer_it);

  auto third_it = third.begin();
  ASSERT_TRUE(third_it != third.end());
  const auto& c_obj = std::get<ubench::sjson::json_object>(*third_it);
  {
    auto member_it = c_obj.begin();
    ASSERT_TRUE(member_it != c_obj.end());
    EXPECT_EQ(std::string_view{(*member_it).key()}, "c");
    EXPECT_TRUE(
        std::holds_alternative<ubench::sjson::json_null>((*member_it).value()));
  }  // Destroying the iterator skips "d", "e", "h", "i" and }.

  ++third_it;
  ASSERT_TRUE(third_it != third.end());
  const auto& j_obj = std::get<ubench::sjson::json_object>(*third_it);
  {
    auto member_it = j_obj.begin();
    ASSERT_TRUE(member_it != j_obj.end());
    EXPECT_EQ(std::string_view{(*member_it).key()}, "j");
  }  // Destroying the iterator skips "k" and "m".

  ++third_it;
  EXPECT_TRUE(third_it == third.end());
  ++outer_it;
  EXPECT_TRUE(outer_it == outer.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, iterate_parent_while_child_iterator_open_throws) {
  ubench::sjson::json_reader reader{"[[1,2],3]"};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& outer = std::get<ubench::sjson::json_array>(*root);

  auto outer_it = outer.begin();
  ASSERT_TRUE(outer_it != outer.end());
  const auto& inner = std::get<ubench::sjson::json_array>(*outer_it);

  auto inner_it = inner.begin();
  ASSERT_TRUE(inner_it != inner.end());

  // The child iterator is still open, so the parent's stack no longer matches.
  EXPECT_THROW(++outer_it, ubench::sjson::json_reader_error);
}

TEST(sjson_reader, array_postfix_increment_returns_previous_element) {
  ubench::sjson::json_reader reader{"[1,\"two\",3]"};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);

  auto it = arr.begin();
  auto previous = it++;
  EXPECT_EQ(
      static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*previous)),
      1);
  EXPECT_EQ(as_view(std::get<ubench::sjson::json_string>(*it)), "two");

  ++it;
  EXPECT_EQ(
      static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*it)), 3);
  ++it;
  EXPECT_TRUE(it == arr.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, array_copy_advances_independently) {
  ubench::sjson::json_reader reader{"[1,2,3]"};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);

  auto it = arr.begin();
  auto copy = it;
  EXPECT_TRUE(it == copy);

  ++it;
  EXPECT_FALSE(it == copy);
  EXPECT_EQ(
      static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*copy)), 1);

  ++copy;
  EXPECT_TRUE(it == copy);
  EXPECT_EQ(
      static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*copy)), 2);

  ++copy;
  ++copy;
  EXPECT_TRUE(copy == arr.end());
  EXPECT_EQ(
      static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*it)), 2);
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, array_copy_of_container_element_is_independent) {
  ubench::sjson::json_reader reader{"[[1,2],[3]]"};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& outer = std::get<ubench::sjson::json_array>(*root);

  auto outer_it = outer.begin();
  auto copy = outer_it;

  {
    const auto& inner = std::get<ubench::sjson::json_array>(*copy);
    auto inner_it = inner.begin();
    ASSERT_TRUE(inner_it != inner.end());
    EXPECT_EQ(
        static_cast<std::int64_t>(std::get<ubench::sjson::json_int>(*inner_it)),
        1);
  }  // The copy's child iterator is closed before the copy advances.

  // The copy's child is bound to the copy, so the original can still advance.
  ++outer_it;
  EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_array>(*outer_it));
  ++copy;
  EXPECT_TRUE(outer_it == copy);
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, array_default_constructed_equals_end) {
  ubench::sjson::json_reader reader{"[1]"};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);

  ubench::sjson::json_array_iterator it{};
  EXPECT_TRUE(it == arr.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, object_postfix_increment_returns_previous_key) {
  ubench::sjson::json_reader reader{R"({"a":1,"b":"x"})"};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& obj = std::get<ubench::sjson::json_object>(*root);

  auto it = obj.begin();
  auto previous = it++;
  EXPECT_EQ(std::string_view{previous->key()}, "a");
  EXPECT_EQ(std::string_view{(*it).key()}, "b");

  ++it;
  EXPECT_TRUE(it == obj.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, object_copy_advances_independently) {
  ubench::sjson::json_reader reader{R"({"a":1,"b":[2],"c":3})"};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& obj = std::get<ubench::sjson::json_object>(*root);

  auto it = obj.begin();
  ++it;
  auto copy = it;
  EXPECT_TRUE(it == copy);

  ++it;
  EXPECT_FALSE(it == copy);
  EXPECT_EQ(std::string_view{(*copy).key()}, "b");

  ++copy;
  EXPECT_TRUE(it == copy);
  EXPECT_EQ(std::string_view{(*copy).key()}, "c");

  ++copy;
  EXPECT_TRUE(copy == obj.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, reader_move_constructed_parses_root) {
  ubench::sjson::json_reader source{R"({"a":[1,2]})"};
  ubench::sjson::json_reader reader{std::move(source)};
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& obj = std::get<ubench::sjson::json_object>(*root);

  auto it = obj.begin();
  ASSERT_TRUE(it != obj.end());
  EXPECT_EQ(std::string_view{it->key()}, "a");
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, reader_move_assigned_parses_root) {
  ubench::sjson::json_reader reader{"[1]"};
  ubench::sjson::json_reader other{R"({"a":2})"};
  reader = std::move(other);
  const auto root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& obj = std::get<ubench::sjson::json_object>(*root);

  auto it = obj.begin();
  ASSERT_TRUE(it != obj.end());
  EXPECT_EQ(std::string_view{it->key()}, "a");
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, reader_move_while_iterating_continues) {
  ubench::sjson::json_reader reader{"[1,2,3]"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);

  auto it = arr.begin();
  ++it;
  ubench::sjson::json_reader moved{std::move(reader)};
  ++it;
  EXPECT_EQ(as_int(*it), 3);
  ++it;
  EXPECT_TRUE(it == arr.end());
  EXPECT_FALSE(moved.has_error());
}

TEST(sjson_reader, reader_moved_from_is_closed_without_error) {
  ubench::sjson::json_reader source{"[1]"};
  ubench::sjson::json_reader reader{std::move(source)};
  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_TRUE(source.is_closed());
  EXPECT_FALSE(source.has_error());
  EXPECT_FALSE(reader.is_closed());
}

TEST(sjson_reader, reader_moved_from_has_no_root) {
  ubench::sjson::json_reader source{"[1]"};
  ubench::sjson::json_reader reader{std::move(source)};
  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(source.root().has_value());
  EXPECT_FALSE(source.has_error());
  EXPECT_TRUE(reader.root().has_value());
}

TEST(sjson_reader, array_iterator_move_constructed_continues) {
  ubench::sjson::json_reader reader{"[1,2,3]"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);

  auto it = arr.begin();
  ++it;
  ubench::sjson::json_array_iterator moved{std::move(it)};
  EXPECT_EQ(as_int(*moved), 2);

  ++moved;
  EXPECT_EQ(as_int(*moved), 3);
  ++moved;
  EXPECT_TRUE(moved == arr.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, array_iterator_move_assigned_continues) {
  ubench::sjson::json_reader reader{"[1,2,3]"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);

  auto it = arr.begin();
  ++it;
  ubench::sjson::json_array_iterator dest{};
  dest = std::move(it);
  EXPECT_EQ(as_int(*dest), 2);

  ++dest;
  EXPECT_EQ(as_int(*dest), 3);
  ++dest;
  EXPECT_TRUE(dest == arr.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, array_move_constructed_iterates) {
  ubench::sjson::json_reader reader{"[1,2,3]"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  ubench::sjson::json_array source =
      std::get<ubench::sjson::json_array>(std::move(*root));
  ubench::sjson::json_array arr{std::move(source)};

  std::int64_t sum{};
  for (const auto& token : arr) sum += as_int(token);
  EXPECT_EQ(sum, 6);
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, array_move_assigned_iterates) {
  ubench::sjson::json_reader reader{"[[1],[2,3]]"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  ubench::sjson::json_array source =
      std::get<ubench::sjson::json_array>(std::move(*root));
  ubench::sjson::json_array arr = source;
  arr = std::move(source);

  std::size_t count{};
  for (const auto& token : arr) {
    EXPECT_TRUE(std::holds_alternative<ubench::sjson::json_array>(token));
    ++count;
  }
  EXPECT_EQ(count, 2u);
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, array_container_element_moved_out_parent_continues) {
  ubench::sjson::json_reader reader{"[[1,2],[3]]"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& outer = std::get<ubench::sjson::json_array>(*root);

  auto outer_it = outer.begin();
  ubench::sjson::json_token first = *outer_it;
  ubench::sjson::json_token moved = std::move(first);

  std::int64_t sum{};
  for (const auto& token : std::get<ubench::sjson::json_array>(moved)) {
    sum += as_int(token);
  }
  EXPECT_EQ(sum, 3);

  // The parent iterator continues after the moved container was consumed.
  ++outer_it;
  ASSERT_TRUE(outer_it != outer.end());
  const auto& second = std::get<ubench::sjson::json_array>(*outer_it);
  auto inner_it = second.begin();
  ASSERT_TRUE(inner_it != second.end());
  EXPECT_EQ(as_int(*inner_it), 3);
  ++inner_it;
  EXPECT_TRUE(inner_it == second.end());
  ++outer_it;
  EXPECT_TRUE(outer_it == outer.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, object_iterator_move_constructed_continues) {
  ubench::sjson::json_reader reader{R"({"a":1,"b":[2],"c":3})"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& obj = std::get<ubench::sjson::json_object>(*root);

  auto it = obj.begin();
  ++it;
  ubench::sjson::json_object_iterator moved{std::move(it)};
  EXPECT_EQ(std::string_view{moved->key()}, "b");

  ++moved;
  EXPECT_EQ(std::string_view{(*moved).key()}, "c");
  ++moved;
  EXPECT_TRUE(moved == obj.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, object_iterator_move_assigned_continues) {
  ubench::sjson::json_reader reader{R"({"a":1,"b":[2],"c":3})"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& obj = std::get<ubench::sjson::json_object>(*root);

  auto it = obj.begin();
  ++it;
  ubench::sjson::json_object_iterator dest{};
  dest = std::move(it);
  EXPECT_EQ(std::string_view{dest->key()}, "b");

  ++dest;
  EXPECT_EQ(std::string_view{dest->key()}, "c");
  ++dest;
  EXPECT_TRUE(dest == obj.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, object_move_constructed_iterates) {
  ubench::sjson::json_reader reader{R"({"a":1,"b":"x"})"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  ubench::sjson::json_object source =
      std::get<ubench::sjson::json_object>(std::move(*root));
  ubench::sjson::json_object obj{std::move(source)};

  std::string keys;
  for (const auto& key : obj) keys += std::string{key.key()};
  EXPECT_EQ(keys, "ab");
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, object_key_moved_keeps_nested_value_iterable) {
  ubench::sjson::json_reader reader{R"({"a":[1,2],"b":3})"};
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& obj = std::get<ubench::sjson::json_object>(*root);

  auto it = obj.begin();
  ubench::sjson::json_key key = *it;
  ubench::sjson::json_key moved = std::move(key);
  EXPECT_EQ(std::string_view{moved.key()}, "a");

  std::int64_t sum{};
  for (const auto& token : std::get<ubench::sjson::json_array>(moved.value())) {
    sum += as_int(token);
  }
  EXPECT_EQ(sum, 3);

  ++it;
  EXPECT_EQ(as_int((*it).value()), 3);
  ++it;
  EXPECT_TRUE(it == obj.end());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, file_reader_parses_root) {
  const temp_file file{"sjson_file_reader_parses_root.json", "[1,2,3]"};
  ubench::sjson::json_reader reader =
      ubench::sjson::json_reader::open(file.path());
  EXPECT_FALSE(reader.has_error());
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);
  auto it = arr.begin();
  EXPECT_EQ(as_int(*it), 1);
  ++it;
  EXPECT_EQ(as_int(*it), 2);
  ++it;
  EXPECT_EQ(as_int(*it), 3);
  ++it;
  EXPECT_TRUE(it == arr.end());
}

TEST(sjson_reader, file_reader_missing_file_throws) {
  const std::filesystem::path missing =
      std::filesystem::temp_directory_path() /
      "sjson_file_reader_missing_does_not_exist.json";
  EXPECT_THROW(ubench::sjson::json_reader::open(missing),
      ubench::sjson::json_reader_error);
}

TEST(sjson_reader, file_reader_empty_file_has_no_root) {
  const temp_file file{"sjson_file_reader_empty.json", ""};
  ubench::sjson::json_reader reader =
      ubench::sjson::json_reader::open(file.path());
  EXPECT_FALSE(reader.root().has_value());
  EXPECT_FALSE(reader.has_error());
}

TEST(sjson_reader, file_reader_root_ends_when_reader_destroyed) {
  const temp_file file{"sjson_file_reader_destroyed.json", "[10,\"abc\"]"};
  std::optional<ubench::sjson::json_token> root;
  {
    ubench::sjson::json_reader reader =
        ubench::sjson::json_reader::open(file.path());
    root = reader.root();
  }
  // Destroying the reader closes it, so returned objects see the end of input.
  std::error_code ec;
  std::filesystem::remove(file.path(), ec);
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);
  EXPECT_TRUE(arr.begin() == arr.end());
}

TEST(sjson_reader, file_reader_moved_continues_iteration) {
  const temp_file file{"sjson_file_reader_moved.json", "[1,2,3]"};
  ubench::sjson::json_reader reader =
      ubench::sjson::json_reader::open(file.path());
  std::optional<ubench::sjson::json_token> root = reader.root();
  ASSERT_OPTIONAL(root);
  const auto& arr = std::get<ubench::sjson::json_array>(*root);
  auto it = arr.begin();
  ubench::sjson::json_reader moved{std::move(reader)};
  EXPECT_EQ(as_int(*it), 1);
  ++it;
  EXPECT_EQ(as_int(*it), 2);
  ++it;
  EXPECT_EQ(as_int(*it), 3);
  EXPECT_FALSE(moved.has_error());
}
