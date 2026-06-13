#include "sjson/json_writer.h"

#include <filesystem>
#include <sstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

TEST(sjson, create_stream) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer(ss);
  EXPECT_NO_THROW(json_writer.close());
}

TEST(sjson, create_file_string) {
  const fs::path tmp = fs::temp_directory_path() / "sjson_writer_test_tmp.json";

  if (fs::exists(tmp)) fs::remove(tmp);

  {
    ubench::sjson::json_writer writer{tmp.string()};
    writer.close();
  }

  EXPECT_TRUE(fs::exists(tmp));
  EXPECT_EQ(fs::file_size(tmp), 0u);

  std::error_code ec;
  fs::remove(tmp, ec);
  EXPECT_FALSE(fs::exists(tmp));
}

TEST(sjson, create_file_string_view) {
  const fs::path tmp =
      fs::temp_directory_path() / "sjson_writer_test_tmp2.json";

  if (fs::exists(tmp)) fs::remove(tmp);

  {
    ubench::sjson::json_writer writer{std::string_view(tmp.string())};
    writer.close();
  }

  EXPECT_TRUE(fs::exists(tmp));
  EXPECT_EQ(fs::file_size(tmp), 0u);

  std::error_code ec;
  fs::remove(tmp, ec);
  EXPECT_FALSE(fs::exists(tmp));
}

TEST(sjson, create_file_path) {
  const fs::path tmp =
      fs::temp_directory_path() / "sjson_writer_test_tmp_path.json";

  if (fs::exists(tmp)) fs::remove(tmp);

  {
    ubench::sjson::json_writer writer{tmp};
    writer.close();
  }

  EXPECT_TRUE(fs::exists(tmp));
  EXPECT_EQ(fs::file_size(tmp), 0u);

  std::error_code ec;
  fs::remove(tmp, ec);
  EXPECT_FALSE(fs::exists(tmp));
}

TEST(sjson, create_stream_close_twice) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};
  EXPECT_NO_THROW(json_writer.close());
  EXPECT_NO_THROW(json_writer.close());
}

TEST(sjson, create_object_array_invalid_sequence) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto o1 = json_writer.write_object();
  EXPECT_THROW({ auto a1 = json_writer.write_array(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_array_object_invalid_sequence) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto a1 = json_writer.write_array();
  EXPECT_THROW({ auto o1 = json_writer.write_object(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_object_object_invalid_sequence) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto o1 = json_writer.write_object();
  EXPECT_THROW({ auto o2 = json_writer.write_object(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_array_array_invalid_sequence) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto a1 = json_writer.write_array();
  EXPECT_THROW({ auto a2 = json_writer.write_array(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_object_array_single_root) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto o1 = json_writer.write_object();
  o1.close();
  EXPECT_THROW({ auto a1 = json_writer.write_array(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_array_object_single_root) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto a1 = json_writer.write_array();
  a1.close();
  EXPECT_THROW({ auto o1 = json_writer.write_object(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_object_object_single_root) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto o1 = json_writer.write_object();
  o1.close();
  EXPECT_THROW({ auto o2 = json_writer.write_object(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_array_array_single_root) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto a1 = json_writer.write_array();
  a1.close();
  EXPECT_THROW({ auto a2 = json_writer.write_array(); },
      ubench::sjson::json_writer_error);
}

TEST(sjson, create_object_array_close_invalid_sequence) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto o1 = json_writer.write_object();
  auto a1 = o1.write_array("array");
  EXPECT_THROW({ o1.close(); }, ubench::sjson::json_writer_error);
}

TEST(sjson, create_array_object_close_invalid_sequence) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto a1 = json_writer.write_array();
  auto o1 = a1.write_object();
  EXPECT_THROW({ a1.close(); }, ubench::sjson::json_writer_error);
}

TEST(sjson, create_object_array_close_root) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto o1 = json_writer.write_object();
  auto a1 = o1.write_array("array");
  json_writer.close();

  // Output is undefined, as the other objects weren't closed first.

  // Any further writes are ignored.
  a1.write_value("test");
}

TEST(sjson, create_array_object_close_root) {
  std::stringstream ss;
  ubench::sjson::json_writer json_writer{ss};

  auto a1 = json_writer.write_array();
  auto o1 = a1.write_object();

  json_writer.close();

  // Output is undefined, as the other objects weren't closed first.

  // Any further writes are ignored.
  o1.write_value("key", "test");
}

TEST(sjson, complex_object) {
  std::stringstream ss;

  // The braces which define scope cause the block to terminate by appending the
  // appropriate array/object end character. Saves having to explicitly write
  // ex.close() because it is closed automatically when going out of scope.
  {
    ubench::sjson::json_writer json_writer{ss};
    {
      auto e1 = json_writer.write_object();
      e1.write_value("host", "netbsd-arm64-rpi4");
      e1.write_value("domain", "home.lan");
      {
        auto e2 = e1.write_object("interfaces");
        {
          auto e3 = e2.write_object("eth0");
          {
            auto e4 = e3.write_array("ipv4");
            e4.write_value("192.168.1.15/24");
            e4.write_value("169.254.234.23/16");
          }
        }
        {
          auto e5 = e2.write_object("lo0");
          {
            auto e6 = e5.write_array("ipv4");
            e6.write_value("127.0.0.1/8");
          }
        }
      }
    }
  }
  EXPECT_EQ(ss.str(),
      R"({ "host": "netbsd-arm64-rpi4", "domain": "home.lan", "interfaces": { "eth0": { "ipv4": [ "192.168.1.15\/24", "169.254.234.23\/16" ] }, "lo0": { "ipv4": [ "127.0.0.1\/8" ] } } })");
}
