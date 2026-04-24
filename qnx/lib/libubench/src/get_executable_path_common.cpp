#include "get_executable_path_common.h"

#include <filesystem>
#include <string>

#include "ubench/os.h"

namespace ubench::os {

auto get_executable_path_name() -> std::string {
  return get_directory_path_name(get_executable_path().string());
}

auto get_directory_path_name(std::string path_name) -> std::string {
  return get_directory_path(std::move(path_name)).string();
}

auto get_directory_path(std::string path_name) -> std::filesystem::path {
  std::filesystem::path p(path_name);
  return p.lexically_normal().parent_path();
}

}  // namespace ubench::os
