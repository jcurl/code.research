#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <ubench/file.h>
#include <ubench/string.h>
#include <fcntl.h>

#include <iostream>

#include "options.h"

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  std::cout << "Device: " << options->device() << std::endl;

  struct statvfs64 stats {};
  int statres = statvfs64(options->device().c_str(), &stats);
  if (statres) {
    ubench::string::perror("statvfs64");
    return 1;
  }

  std::cout << "Preferred filesystem block size = " << stats.f_bsize
            << std::endl;
  std::cout << "Fundamental filesystem block size = " << stats.f_frsize
            << std::endl;
  std::cout << "Total number of blocks = " << stats.f_blocks << std::endl;
  std::cout << "Total number of free blocks = " << stats.f_bfree << std::endl;
  std::cout << "Total number of (user) free blocks = " << stats.f_bavail
            << std::endl;
  std::cout << "Total number of file inodes = " << stats.f_files << std::endl;
  std::cout << "Total number of free file inodes = " << stats.f_ffree
            << std::endl;
  std::cout << "Total number of (user) free file inodes = " << stats.f_favail
            << std::endl;
  std::cout << "Filesystem ID = 0x" << std::hex << stats.f_fsid << std::dec
            << std::endl;
  std::cout << "Filesystem base type = " << stats.f_basetype << std::endl;
  std::cout << "Flags = 0x" << std::hex << stats.f_flag << std::dec
            << std::endl;
  std::cout << "Maximum file name length = " << stats.f_namemax << std::endl;

  return 0;
}
