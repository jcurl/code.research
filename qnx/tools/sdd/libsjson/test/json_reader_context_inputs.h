#ifndef SJSON_TEST_JSON_READER_CONTEXT_INPUTS_H
#define SJSON_TEST_JSON_READER_CONTEXT_INPUTS_H

#include <array>
#include <string_view>

namespace ubench::sjson::detail::valid_inputs {

inline constexpr std::string_view empty{""};
inline constexpr std::string_view only_comment{"// nothing but a comment"};

// Comments are skipped wherever they appear.
inline constexpr std::string_view comment_at_start{"// leading comment\n[1]"};
inline constexpr std::string_view comment_in_middle{"[ // inner comment\n1 ]"};
inline constexpr std::string_view comment_after_line{"[1] // trailing comment"};

// The same tokens must be produced regardless of the line ending.
inline constexpr std::string_view newlines_cr{"// comment\r[\r1,\r\"a\"\r]\r"};
inline constexpr std::string_view newlines_crlf{
    "// comment\r\n[\r\n1,\r\n\"a\"\r\n]\r\n"};
inline constexpr std::string_view newlines_lf{"// comment\n[\n1,\n\"a\"\n]\n"};

// Covers every character std::isspace accepts in the C locale.
inline constexpr std::string_view whitespace{
    " \t\v[\f1\t,\t2\r\n,\v\r\n3 \f]\t\r\n\v "};

inline constexpr std::string_view punctuation{"{\"k\":[[],{}]}"};
inline constexpr std::string_view keywords{"[true,false,null]"};
inline constexpr std::string_view integers{"[0,42,-7,9223372036854775807]"};
inline constexpr std::string_view strings{
    "[\"\",\"abc\",\"a b\",\"http://x\"]"};
// "é" (U+00E9), "😀" (U+1F600) and "a" are well-formed UTF-8.
inline constexpr std::string_view utf8_strings{
    "[\"\303\251\",\"\360\237\230\200\",\"a\"]"};
inline constexpr std::string_view plus_prefixed_integers{
    "[+0,+42,+9223372036854775807]"};
inline constexpr std::string_view array_of_integers_trailing_comma{"[1,2,3,]"};
inline constexpr std::string_view object_of_integers_trailing_comma{
    "{\"a\":1,\"b\":2,\"c\":3,}"};

inline constexpr std::string_view escaped_quotes_in_string{
    "[\"say \\\"hi\\\"\"]"};
inline constexpr std::string_view single_quoted_strings{"['a','b c']"};
inline constexpr std::string_view escaped_backslash_in_string{"[\"a\\\\b\"]"};
inline constexpr std::string_view escaped_slash_in_string{"[\"a\\/b\"]"};

// A quote that differs from the delimiter needs no escape.
inline constexpr std::string_view unescaped_double_quote_in_single_quoted{
    "['say \"hi\"']"};
inline constexpr std::string_view unescaped_single_quote_in_double_quoted{
    "[\"it's\"]"};
// Escaping a quote that differs from the delimiter is optional but allowed.
inline constexpr std::string_view escaped_double_quote_in_single_quoted{
    "['a\\\"b']"};
inline constexpr std::string_view escaped_single_quote_in_double_quoted{
    "[\"it\\'s\"]"};

inline constexpr std::string_view nested_arrays{"[[1,[2,[3]]],[]]"};
inline constexpr std::string_view nested_objects{
    "{\"a\":{\"b\":{\"c\":null}}}"};
inline constexpr std::string_view mixed_types{
    "[1,\"two\",true,false,null,{\"k\":[]}]"};
inline constexpr std::string_view array_of_objects_with_arrays{
    "[{\"a\":[1,{\"b\":[]}]},{\"c\":[true]}]"};
inline constexpr std::string_view object_of_arrays_with_objects{
    "{\"x\":[{\"y\":[null,2]},{}],\"z\":[[]]}"};

inline constexpr std::string_view every_token{R"(
[
  {
    "a": [
      1,
      "two",
      true,
      null,
      {
        "k": []
      },
      [
        3
      ]
    ]
  },
  {
    "b": [
      false,
      [],
      {},
      's',
      -4
    ]
  },
  [
    {
      "c": null,
      "d": [
        4
      ],
      "e": {
        "f": "g"
      },
      "h": false,
      "i": +7
    },
    {
      "j": true,
      "k": 'l',
      "m": [
        null
      ]
    }
  ]
]
)"};

}  // namespace ubench::sjson::detail::valid_inputs

namespace ubench::sjson::detail::invalid_inputs {

// Each of these must produce std::nullopt on the first call.
inline constexpr std::array<std::string_view, 18> invalid_sequences{
    "@",
    "\"unterminated",
    "tru",
    "fals",
    "nul",
    "-",
    "/",
    "/x",
    "@}}}",
    "}}}@",
    // Floats not supported
    ".5",
    ".5e3",
    "-.5",
    "+1.5",
    "1.5",
    "-0.5",
    "1e10",
    "2E+3",
};

// Well-formed integers whose values do not fit in std::int64_t. The tokenizer
// returns them as integer tokens; get_token() must report an error.
inline constexpr std::array<std::string_view, 3> out_of_range_integers{
    "[18446744073709551616]",
    "[-9223372036854775809]",
    "[123456789012345678901234567890]",
};

// Strings that are not well-formed UTF-8: a stray continuation byte, an
// overlong encoding, an encoded surrogate, a code point above U+10FFFF, a
// truncated sequence, an invalid continuation byte, and a 0xFF byte. The
// tokenizer returns them as string tokens; get_token() must report an error.
inline constexpr std::array<std::string_view, 7> invalid_utf8_strings{
    "[\"\200\"]",
    "[\"\300\257\"]",
    "[\"\355\240\200\"]",
    "[\"\364\220\200\200\"]",
    "[\"\342\202\"]",
    "[\"\303(\"]",
    "[\"\377\"]",
};

// Tokens before the invalid sequence are returned, then std::nullopt.
inline constexpr std::string_view invalid_after_token{"[ @ ]"};

}  // namespace ubench::sjson::detail::invalid_inputs

namespace ubench::sjson::detail::invalid_grammar {

// Valid tokens arranged in sequences that break the grammar. Earlier tokens may
// be returned before the error is reported.

// <root> := <container>
inline constexpr std::string_view root_int{"1"};
inline constexpr std::string_view root_string{"\"a\""};
inline constexpr std::string_view root_true{"true"};
inline constexpr std::string_view root_false{"false"};
inline constexpr std::string_view root_null{"null"};
inline constexpr std::string_view root_two_arrays{"[] []"};
inline constexpr std::string_view root_two_objects{"{}{}"};
inline constexpr std::string_view root_array_object{"[]{}"};
inline constexpr std::string_view root_array_int{"[]1"};
inline constexpr std::string_view root_object_string{"{}\"a\""};
inline constexpr std::string_view root_extra_close{"[]]"};

// <array> := array_open ( <json> ( comma <json> )* comma? ) array_close
inline constexpr std::string_view arr_no_comma{"[1 2]"};
inline constexpr std::string_view arr_double_comma{"[1,,2]"};
inline constexpr std::string_view arr_leading_comma{"[,1]"};
inline constexpr std::string_view arr_only_comma{"[,]"};
inline constexpr std::string_view arr_array_no_comma{"[[] []]"};
inline constexpr std::string_view arr_object_no_comma{"[{} {}]"};
inline constexpr std::string_view arr_unclosed{"[1"};
inline constexpr std::string_view arr_unclosed_nested{"[[1]"};
inline constexpr std::string_view arr_extra_close{"[1]]"};
inline constexpr std::string_view arr_mismatch_close{"[1}"};
inline constexpr std::string_view arr_member{"[\"a\":1]"};
inline constexpr std::string_view arr_colon{"[1:2]"};
inline constexpr std::string_view arr_nested_member{"[[\"a\":1]]"};
inline constexpr std::string_view arr_object_mismatch{"[{\"a\":1]"};

// <object> := object_open ( <member> ( comma <member> )* comma? ) object_close
// <member> := string object_colon <json>
inline constexpr std::string_view obj_no_colon{"{\"a\" 1}"};
inline constexpr std::string_view obj_no_value{"{\"a\":}"};
inline constexpr std::string_view obj_no_key{"{:1}"};
inline constexpr std::string_view obj_colon_only{"{:}"};
inline constexpr std::string_view obj_int_key{"{1:2}"};
inline constexpr std::string_view obj_null_key{"{null:1}"};
inline constexpr std::string_view obj_bool_key{"{true:1}"};
inline constexpr std::string_view obj_array_key{"{[1]:2}"};
inline constexpr std::string_view obj_value_no_key{"{1}"};
inline constexpr std::string_view obj_key_no_colon{"{\"a\"}"};
inline constexpr std::string_view obj_no_comma{"{\"a\":1 \"b\":2}"};
inline constexpr std::string_view obj_double_comma{"{\"a\":1,,\"b\":2}"};
inline constexpr std::string_view obj_leading_comma{"{,\"a\":1}"};
inline constexpr std::string_view obj_double_colon{"{\"a\"::1}"};
inline constexpr std::string_view obj_nested_no_colon{"{\"a\":{\"b\" 1}}"};
inline constexpr std::string_view obj_unclosed{"{\"a\":1"};
inline constexpr std::string_view obj_unclosed_nested{"{\"a\":{\"b\":1}"};
inline constexpr std::string_view obj_mismatch_close{"{\"a\":1]"};
inline constexpr std::string_view obj_extra_close{"{\"a\":1}}"};

// Containers of different kinds combined incorrectly.
inline constexpr std::string_view mix_arr_in_obj_unclosed{"{\"a\":[1}"};
inline constexpr std::string_view mix_obj_in_arr_no_comma{"[{\"a\":1}2]"};
inline constexpr std::string_view mix_arr_then_member{"{\"a\":[]\"b\":1}"};

}  // namespace ubench::sjson::detail::invalid_grammar

#endif
