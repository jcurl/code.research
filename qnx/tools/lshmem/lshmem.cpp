#include <unistd.h>

#include <iostream>
#include <map>
#include <sstream>

#include "osqnx/pids.h"
#include "ubench/string.h"
#include "options.h"
#include "pid_mapping.h"
#include "shmem_map.h"

auto mapping_file(unsigned int pid) {
  std::ostringstream pathstream{};
  pathstream << "/proc/" << pid << "/mappings";
  return pathstream.str();
}

auto print_pid(const std::vector<unsigned int>& pids, unsigned int pid)
    -> bool {
  if (pids.empty()) return true;
  return std::find(pids.cbegin(), pids.cend(), pid) != pids.cend();
}

auto get_shmem(const std::vector<unsigned int>& pids, class pids& p,
    int verbosity, bool read) -> std::map<unsigned int, pid_mapping> {
  std::map<unsigned int, pid_mapping> shmem{};

  unsigned int self = getpid();
  for (auto pid : pids) {
    if (pid == self) continue;
    auto pid_name = p.get_name(pid);
    if (verbosity) {
      if (pid_name) {
        std::cout << "Querying PID: " << pid << " (" << *pid_name << ")... "
                  << std::flush;
      } else {
        std::cout << "Querying PID: " << pid << "... " << std::flush;
      }
    }
    auto file = mapping_file(pid);
    auto mapping = load_mapping(file, read);
    if (!mapping) {
      std::cerr << "Error: File: " << file << "; "
                << ubench::string::perror(mapping.error()) << std::endl;
    } else {
      shmem.try_emplace(pid, *mapping);
      if (verbosity) {
        std::cout << "done." << std::endl;
      }
    }
  }
  return shmem;
}

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  pids p{};
  std::map<unsigned int, pid_mapping> shmem{};
  if (options->pids().empty() || options->shared_mem()) {
    shmem =
        get_shmem(p.query_pids(), p, options->verbosity(), options->readable());
  } else {
    shmem = get_shmem(
        options->pids(), p, options->verbosity(), options->readable());
  }

  for (const auto& [pid, mapping] : shmem) {
    if (!print_pid(options->pids(), pid)) continue;
    auto pid_name = p.get_name(pid);
    if (pid_name) {
      std::cout << "PID: " << std::dec << pid << " (" << *pid_name << ")... "
                << std::endl;
    } else {
      std::cout << "PID: " << std::dec << pid << "... " << std::endl;
    }

    if (options->phys_mem()) {
      bool header = true;
      for (const auto& m : mapping.map()) {
        if (header) {
          std::cout << " Shared Physical Memory Map" << std::endl;
          header = false;
        }
        std::cout << "  " << std::hex << m.phys_addr << "," << m.phys_len << ","
                  << m.ri_flags << "," << m.ri_prot << "," << m.dev << ","
                  << m.ino << "," << m.object << std::dec << std::endl;
      }
    }

    if (options->shared_mem()) {
      bool header = true;
      for (const auto& [pid2, mapping2] : shmem) {
        if (pid != pid2) {
          shmem_map overlap{};
          auto i1 = mapping.map().cbegin();
          auto i2 = mapping2.map().cbegin();
          while (i1 != mapping.map().cend() && i2 != mapping2.map().cend()) {
            overlap.compare_and_add(*i1, *i2);
            if (i1->phys_addr + i1->phys_len < i2->phys_addr + i2->phys_len) {
              i1 = std::next(i1);
            } else if (i1->phys_addr + i1->phys_len >
                       i2->phys_addr + i2->phys_len) {
              i2 = std::next(i2);
            } else {
              i1 = std::next(i1);
              i2 = std::next(i2);
            }
          }

          for (const auto& entry : overlap.overlap()) {
            if (header) {
              std::cout << " Overlapping Shared Memory Map" << std::endl;
              header = false;
            }
            std::cout << "  (" << std::hex << entry.p1.ri_flags << ","
                      << entry.p1.ri_prot << "," << entry.p1.dev << ","
                      << entry.p1.ino << ") <-> " << std::dec << pid2 << ": ("
                      << std::hex << entry.p1.ri_flags << ","
                      << entry.p1.ri_prot << "," << entry.p1.dev << ","
                      << entry.p1.ino << ") [" << entry.p1.object;
            if (entry.p1.object == entry.p2.object) {
              std::cout << "] length=";
            } else {
              std::cout << "," << entry.p2.object << "] length=";
            }
            std::cout << entry.size << std::endl;
          }
        }
      }
    }
  }

  return 0;
}
