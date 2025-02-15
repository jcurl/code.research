#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>

#include "osqnx/asinfo.h"
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
  // The list of pids may not be sorted.
  if (pids.empty()) return true;
  return std::find(pids.cbegin(), pids.cend(), pid) != pids.cend();
}

using pid_shmem_map = std::map<unsigned int, pid_mapping>;

auto get_shmem(const std::vector<unsigned int>& pids, os::qnx::pids& p,
    int verbosity, bool read) -> pid_shmem_map {
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

auto print_tymem() -> void {
  auto asinfo = os::qnx::get_asinfo();
  if (asinfo.empty()) return;

  std::cout << "Typed Memory (Sorted):" << std::endl;
  std::cout << std::hex;
  for (const auto& entry : asinfo) {
    std::cout << std::setfill('0') << std::setw(16) << entry.start << " - "
              << std::setfill('0') << std::setw(16) << entry.end << ": "
              << entry.name;
    if (entry.end + 1 < entry.start) {
      std::cout << "  ** WARNING: End is before Start.";
    }
    std::cout << std::endl;
  }
  std::cout << std::dec;
}

auto print_mem_map(const pid_mapping& mapping) -> void {
  bool header = true;
  for (const auto& m : mapping.map()) {
    if (header) {
      std::cout << " Shared Physical Memory Map" << std::endl;
      header = false;
    }
    std::cout << "  " << std::hex << m.phys_addr << "," << m.phys_len << ","
              << m.ri_flags << "," << m.ri_prot << "," << m.dev << "," << m.ino
              << "," << m.object << std::dec;
    auto tmem = os::qnx::get_tymem_name(m.phys_addr);
    if (tmem) {
      std::cout << "," << *tmem;
    }
    std::cout << std::endl;
  }
}

auto print_shmem(const pid_shmem_map& shmem, unsigned int pid,
    const pid_mapping& mapping) -> void {
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
                  << std::hex << entry.p1.ri_flags << "," << entry.p1.ri_prot
                  << "," << entry.p1.dev << "," << entry.p1.ino << ") ["
                  << entry.p1.object;
        if (entry.p1.object == entry.p2.object) {
          std::cout << "] length=";
        } else {
          std::cout << "," << entry.p2.object << "] length=";
        }
        std::cout << entry.size;
        if (!entry.tymem.empty()) {
          bool first = true;
          std::cout << " tymem=";
          for (const auto name : entry.tymem) {
            if (!first) std::cout << ",";
            std::cout << name;
            first = false;
          }
        }
        std::cout << std::endl;
      }
    }
  }
}

auto main(int argc, char* argv[]) -> int {
  auto options = make_options(argc, argv);
  if (!options) return options.error();

  if (options->tymem()) {
    print_tymem();
  }

  if (!options->phys_mem() && !options->shared_mem()) {
    return 0;
  }
  if (options->tymem()) {
    std::cout << std::endl;
  }

  os::qnx::pids p{};
  pid_shmem_map shmem{};
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
      print_mem_map(mapping);
    }

    if (options->shared_mem()) {
      print_shmem(shmem, pid, mapping);
    }
  }

  return 0;
}
