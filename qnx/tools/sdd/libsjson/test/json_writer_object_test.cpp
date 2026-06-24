#include <sstream>

#include <gtest/gtest.h>

#include "sjson/json_writer.h"

TEST(sjson, create_object_empty) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto o1 = json_writer.write_object();
  o1.close();
  json_writer.close();

  EXPECT_EQ(ss.str(), "{ }");
}

TEST(sjson, create_object_close_twice) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_object();
  a1.close();
  EXPECT_EQ(ss.str(), "{ }");

  a1.close();
  EXPECT_EQ(ss.str(), "{ }");

  json_writer.close();
  EXPECT_EQ(ss.str(), "{ }");
}

TEST(sjson, create_null_value) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);
  auto e1 = json_writer.write_object();
  e1.write_value("", nullptr);
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "": null })");
}

TEST(sjson, escape_value_key_quote_middle) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value(R"( "a" )", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ " \"a\" ": "value" })");
}

TEST(sjson, escape_value_key_quote_beginning_end) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value(R"("a")", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "\"a\"": "value" })");
}

TEST(sjson, escape_value_key_backslash) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value(R"(\usr)", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "\\usr": "value" })");
}

TEST(sjson, escape_value_key_forwardslash) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("/usr", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "\/usr": "value" })");
}

TEST(sjson, escape_value_key_backspace) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("\b", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "\b": "value" })");
}

TEST(sjson, escape_value_key_newline) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("Key\n1", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "Key\n1": "value" })");
}

TEST(sjson, escape_value_key_newline_cr) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("key\n\r1", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "key\n\r1": "value" })");
}

TEST(sjson, escape_value_key_tab) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("point\t", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "point\t": "value" })");
}

TEST(sjson, escape_value_key_formfeed) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("key 1\fkey 2", "value");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "key 1\fkey 2": "value" })");
}

TEST(sjson, escape_value_object_euro) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("units", "€");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "units": "€" })");
}

TEST(sjson, escape_value_key_euro) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_object();
  e1.write_value("€", "1.12");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"({ "€": "1.12" })");
}

TEST(sjson, write_object_array_wrong_sequence_string) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto o1 = json_writer.write_object();
  auto a1 = o1.write_array("array");
  EXPECT_THROW(
      { o1.write_value("key", "value"); }, ubench::sjson::json_writer_error);
}

TEST(sjson, write_object_array_wrong_sequence_true) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto o1 = json_writer.write_object();
  auto a1 = o1.write_array("array");
  EXPECT_THROW(
      { o1.write_value("key", true); }, ubench::sjson::json_writer_error);
}

TEST(sjson, write_object_array_wrong_sequence_null) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto o1 = json_writer.write_object();
  auto a1 = o1.write_array("array");
  EXPECT_THROW({ o1.write_null("key"); }, ubench::sjson::json_writer_error);
}

TEST(sjson, write_object_array_wrong_sequence_int) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto o1 = json_writer.write_object();
  auto a1 = o1.write_array("array");
  EXPECT_THROW(
      { o1.write_value("key", 128); }, ubench::sjson::json_writer_error);
}
