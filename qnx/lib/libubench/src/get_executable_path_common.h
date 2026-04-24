#ifndef UBENCH_OS_GET_EXECUTABLE_PATH_COMMON_H
#define UBENCH_OS_GET_EXECUTABLE_PATH_COMMON_H

#include <filesystem>
#include <string>

namespace ubench::os {

auto get_directory_path_name(std::string path_name) -> std::string;
auto get_directory_path(std::string path_name) -> std::filesystem::path;

}  // namespace ubench::os

#endif
