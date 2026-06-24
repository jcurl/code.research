#include <sstream>

#include <gtest/gtest.h>

#include "sjson/json_writer.h"

TEST(sjson, create_array_empty) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.close();
  json_writer.close();

  EXPECT_EQ(ss.str(), "[ ]");
}

TEST(sjson, create_array_close_twice) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.close();
  EXPECT_EQ(ss.str(), "[ ]");

  a1.close();
  EXPECT_EQ(ss.str(), "[ ]");

  json_writer.close();
  EXPECT_EQ(ss.str(), "[ ]");
}

TEST(sjson, array_string) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_value("test");
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ \"test\" ]");
}

TEST(sjson, array_string_string) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_value("test");
  a1.write_value("foo");
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ \"test\", \"foo\" ]");
}

TEST(sjson, array_number) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_value(10);
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ 10 ]");
}

TEST(sjson, array_number_number) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_value(10);
  a1.write_value(-42);
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ 10, -42 ]");
}

TEST(sjson, array_bool_true) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_value(true);
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ true ]");
}

TEST(sjson, array_bool_false) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_value(false);
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ false ]");
}

TEST(sjson, array_null) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_null();
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ null ]");
}

TEST(sjson, array_bool_null) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);

  auto a1 = json_writer.write_array();
  a1.write_value(false);
  a1.write_null();
  a1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), "[ false, null ]");
}

TEST(sjson, create_null_array_element) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);
  auto e1 = json_writer.write_array();
  e1.write_value(nullptr);
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ null ])");
}

TEST(sjson, escape_value_array_quote_middle) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value(R"(This is "a" test)");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "This is \"a\" test" ])");
}

TEST(sjson, escape_value_array_quote_beginning_end) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value(R"("This is "a" test")");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "\"This is \"a\" test\"" ])");
}

TEST(sjson, escape_value_array_backslash) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value(R"(c:\tmp.txt)");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "c:\\tmp.txt" ])");
}

TEST(sjson, escape_value_array_forwardslash) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value("/usr/bin/ls");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "\/usr\/bin\/ls" ])");
}

TEST(sjson, escape_value_array_forwardslash_disabled) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  json_writer.config().escape_solidus = false;
  auto e1 = json_writer.write_array();
  e1.write_value("/usr/bin/ls");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "/usr/bin/ls" ])");
}

TEST(sjson, escape_value_array_backspace) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value("Error\b\b");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "Error\b\b" ])");
}

TEST(sjson, escape_value_array_newline) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value("My\nspace");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "My\nspace" ])");
}

TEST(sjson, escape_value_array_newline_cr) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value("Log\n\rline\n\r");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "Log\n\rline\n\r" ])");
}

TEST(sjson, escape_value_array_tab) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value("\tpoint 1");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "\tpoint 1" ])");
}

TEST(sjson, escape_value_array_formfeed) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto e1 = json_writer.write_array();
  e1.write_value("Page 1\fPage 2");
  e1.close();
  json_writer.close();
  EXPECT_EQ(ss.str(), R"([ "Page 1\fPage 2" ])");
}

TEST(sjson, write_array_object_wrong_sequence_string) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto a1 = json_writer.write_array();
  auto o1 = a1.write_object();
  EXPECT_THROW({ a1.write_value("value"); }, ubench::sjson::json_writer_error);
}

TEST(sjson, write_array_object_wrong_sequence_true) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto a1 = json_writer.write_array();
  auto o1 = a1.write_object();
  EXPECT_THROW({ a1.write_value(true); }, ubench::sjson::json_writer_error);
}

TEST(sjson, write_array_object_wrong_sequence_null) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto a1 = json_writer.write_array();
  auto o1 = a1.write_object();
  EXPECT_THROW({ a1.write_null(); }, ubench::sjson::json_writer_error);
}

TEST(sjson, write_array_object_wrong_sequence_int) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  auto a1 = json_writer.write_array();
  auto o1 = a1.write_object();
  EXPECT_THROW({ a1.write_value(128); }, ubench::sjson::json_writer_error);
}

TEST(sjson, write_array_uint8) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::uint8_t>(255));
    }
  }
  EXPECT_EQ(ss.str(), "[ 255 ]");
}

TEST(sjson, write_array_uint16) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::uint16_t>(65535));
    }
  }
  EXPECT_EQ(ss.str(), "[ 65535 ]");
}

TEST(sjson, write_array_uint32) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::uint32_t>(0xFFFFFFFF));
    }
  }
  EXPECT_EQ(ss.str(), "[ 4294967295 ]");
}

TEST(sjson, write_array_uint64) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::uint64_t>(0xFFFFFFFFFFFFFFFF));
    }
  }
  EXPECT_EQ(ss.str(), "[ 18446744073709551615 ]");
}

TEST(sjson, write_array_int8) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::int8_t>(-127));
    }
  }
  EXPECT_EQ(ss.str(), "[ -127 ]");
}

TEST(sjson, write_array_int16) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::int16_t>(-32768));
    }
  }
  EXPECT_EQ(ss.str(), "[ -32768 ]");
}

TEST(sjson, write_array_int32) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::int32_t>(0x80000001));
    }
  }
  EXPECT_EQ(ss.str(), "[ -2147483647 ]");
}

TEST(sjson, write_array_int64) {
  std::stringstream ss;
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto a1 = json_writer.write_array();
      a1.write_value(static_cast<std::int64_t>(0x8000000000000001));
    }
  }
  EXPECT_EQ(ss.str(), "[ -9223372036854775807 ]");
}
