#include "ubench/measure/print.h"

#include <sstream>

#include <gtest/gtest.h>

TEST(print_table, empty) {
  std::stringstream os{};
  ubench::measure::table t{};

  os << t << std::flush;
  EXPECT_EQ(os.str(), "");
}

TEST(print_table, one_column_no_data) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1");

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| ----- |\n|       |");
}

TEST(print_table, one_column_data_left_1) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1");
  t.add_line({"x"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| ----- |\n| x     |");
}

TEST(print_table, one_column_data_left_4) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1");
  t.add_line({"xxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| ----- |\n| xxxx  |");
}

TEST(print_table, one_column_data_left_5) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1");
  t.add_line({"xxxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| ----- |\n| xxxxx |");
}

TEST(print_table, one_column_data_left_6) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1");
  t.add_line({"xxxxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1  |\n| ------ |\n| xxxxxx |");
}

TEST(print_table, one_column_data_right_1) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::right);
  t.add_line({"x"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| ----: |\n|     x |");
}

TEST(print_table, one_column_data_right_4) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::right);
  t.add_line({"xxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| ----: |\n|  xxxx |");
}

TEST(print_table, one_column_data_right_5) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::right);
  t.add_line({"xxxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| ----: |\n| xxxxx |");
}

TEST(print_table, one_column_data_right_6) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::right);
  t.add_line({"xxxxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "|  col 1 |\n| -----: |\n| xxxxxx |");
}

TEST(print_table, one_column_data_centre_1) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::centre);
  t.add_line({"x"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| :---: |\n|   x   |");
}

TEST(print_table, one_column_data_centre_4) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::centre);
  t.add_line({"xxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| :---: |\n| xxxx  |");
}

TEST(print_table, one_column_data_centre_5) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::centre);
  t.add_line({"xxxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1 |\n| :---: |\n| xxxxx |");
}

TEST(print_table, one_column_data_centre_6) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::centre);
  t.add_line({"xxxxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| col 1  |\n| :----: |\n| xxxxxx |");
}

TEST(print_table, one_column_data_centre_7) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_column("col 1", ubench::measure::alignment::centre);
  t.add_line({"xxxxxxx"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "|  col 1  |\n| :-----: |\n| xxxxxxx |");
}

TEST(print_table, no_header) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_line({"x"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| x |");
}

TEST(print_table, no_header_two_cols) {
  std::stringstream os{};
  ubench::measure::table t{};
  t.add_line({"x", "y"});

  os << t << std::flush;
  EXPECT_EQ(os.str(), "| x | y |");
}
