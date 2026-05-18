#include "dump_amd.h"

#include <optional>
#include <vector>

#include "dump_generic.h"

namespace rjcp::cpuid {

namespace {

auto dump_amd_normal(cpuidreader& reader, std::vector<cpuid_info>& registers)
    -> bool {
  cpuidreg leaf{};
  cpuidreg maxleaf{};

  bool success{};

  while (true) {
    auto reg = get_cpuid(reader, leaf, 0);
    if (reg) {
      success = true;
      registers.push_back(*reg);
      switch (leaf) {
        case 0x00: {
          // Maximum number of leaves and branding
          maxleaf = reg->res.eax;
          break;
        }
        case 0x07: {
          // Structured Extended Feature Flags.
          cpuidreg subleaves = reg->res.eax;
          for (cpuidreg subleaf = 1; subleaf <= subleaves; subleaf++) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            // Clang 18.1.3 false warning.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            registers.push_back(*reg);
          }
          break;
        }
        case 0x0D: {
          // Processor Extended State
          for (cpuidreg subleaf = 1; subleaf < 64; subleaf++) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (reg && (subleaf < 2 || (reg->res.eax || reg->res.ebx ||
                                           reg->res.ecx || reg->res.edx))) {
              // Store this entry, as it's interesting.
              registers.push_back(*reg);
            }
          }
          break;
        }
        default:
          // We've already added the element to the vector.
          break;
      }
    }

    // If we couldn't get CPUID.0h, then `maxleaf` is zero, and we return with
    // no data.
    if (leaf == maxleaf) return success;
    leaf++;
  }
}

auto dump_amd_extended(cpuidreader& reader, std::vector<cpuid_info>& registers)
    -> bool {
  cpuidreg leaf{0x80000000};
  cpuidreg maxleaf{leaf};

  bool success{};

  while (true) {
    auto reg = get_cpuid(reader, leaf, 0);
    if (reg) {
      success = true;
      registers.push_back(*reg);
      switch (leaf) {
        case 0x80000000: {
          // Maximum number of leaves and branding.
          maxleaf = reg->res.eax;
          break;
        }
        case 0x8000001D: {
          cpuidreg subleaf = 1;
          // Clang 18.1.3 false warning.
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          while (subleaf <= 0xFF && (reg->res.eax & 0x1F)) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            // Clang 18.1.3 false warning.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            registers.push_back(*reg);
            subleaf++;
          }
          break;
        }
        case 0x80000020: {
          for (cpuidreg subleaf : {1, 2, 3, 5}) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            // Clang 18.1.3 false warning.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            registers.push_back(*reg);
          }
          break;
        }
        case 0x80000026: {
          cpuidreg subleaf = 1;
          // Clang 18.1.3 false warning.
          // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
          while (subleaf <= 0xFF && (reg->res.ecx & 0xFF00)) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            // Clang 18.1.3 false warning.
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            registers.push_back(*reg);
            subleaf++;
          }
          break;
        }
        default:
          // We've already added the element to the vector.
          break;
      }
    }

    // If we couldn't get CPUID.0h, then `maxleaf` is zero, and we return with
    // no data.
    if (leaf == maxleaf) return success;
    leaf++;
  }
}

}  // namespace

auto dump_cpu_amd(cpuidreader& reader)
    -> std::optional<std::vector<cpuid_info>> {
  std::vector<cpuid_info> registers{};
  bool success = dump_amd_normal(reader, registers);
  success = success && dump_amd_extended(reader, registers);
  success = success && dump_hypervisor(reader, registers);

  if (!success) return std::nullopt;
  return registers;
}

}  // namespace rjcp::cpuid
