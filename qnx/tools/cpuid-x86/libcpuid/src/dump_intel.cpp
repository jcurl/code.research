#include "dump_intel.h"

#include <optional>
#include <vector>

#include "dump_generic.h"

namespace rjcp::cpuid {

namespace {

auto dump_intel_normal(cpuidreader& reader, std::vector<cpuid_info>& registers)
    -> bool {
  cpuidreg leaf{};
  cpuidreg maxleaf{};

  bool sgx{false};
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
        case 0x02: {
          // Cache descriptors
          cpuidreg subleaves = reg->res.eax & 0xFF;
          for (cpuidreg subleaf = 1; subleaf < subleaves; subleaf++) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            registers.push_back(*reg);
          }
          break;
        }
        case 0x07: {
          // Structured Extended Feature Flags.
          cpuidreg subleaves = reg->res.eax;
          sgx = reg->res.ebx & 0x04;
          for (cpuidreg subleaf = 1; subleaf <= subleaves; subleaf++) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            registers.push_back(*reg);
          }
          break;
        }
        case 0x0B:
        case 0x1F: {
          // x2APIC features
          cpuidreg subleaf = 1;
          while (reg->res.ebx & 0xFFFF) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            registers.push_back(*reg);
            subleaf++;
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
        case 0x0F: {
          reg = get_cpuid(reader, leaf, 1);
          if (!reg) break;
          registers.push_back(*reg);
          break;
        }
        case 0x10: {
          cpuidreg subleaf = 1;
          cpuidreg residbit = reg->res.ebx >> 1;
          while (residbit) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            registers.push_back(*reg);
            residbit >>= 1;
            subleaf++;
          }
          break;
        }
        case 0x12: {
          if (sgx) {
            cpuidreg subleaf = 1;
            while (subleaf <= 2 || (subleaf <= 0xFF && (reg->res.eax & 0xF))) {
              reg = get_cpuid(reader, leaf, subleaf);
              if (!reg) break;
              registers.push_back(*reg);
              subleaf++;
            }
          }
          break;
        }
        case 0x14:
        case 0x17:
        case 0x18:
        case 0x20: {
          cpuidreg subleaves = reg->res.eax;
          for (cpuidreg subleaf = 1; subleaf <= subleaves; subleaf++) {
            reg = get_cpuid(reader, leaf, subleaf);
            if (!reg) break;
            registers.push_back(*reg);
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

auto dump_cpu_intel(cpuidreader& reader)
    -> std::optional<std::vector<cpuid_info>> {
  std::vector<cpuid_info> registers{};
  bool success = dump_intel_normal(reader, registers);
  success = success && dump_range(reader, 0x80000000, registers);
  success = success && dump_range(reader, 0x20000000, registers);
  success = success && dump_hypervisor(reader, registers);

  if (!success) return std::nullopt;
  return registers;
}

}  // namespace rjcp::cpuid