#include "dump_generic.h"

namespace rjcp::cpuid {

auto get_cpuid(cpuidreader& reader, cpuidreg eax, cpuidreg ecx)
    -> std::optional<cpuid_info> {
  cpuid_info result{cpuid_req{eax, ecx}, cpuid_res{}};
  auto reg = reader.cpuid(eax, ecx);
  if (!reg) return std::nullopt;
  result.res = *reg;
  return result;
}

auto dump_range(cpuidreader& reader, cpuidreg base,
    std::vector<cpuid_info>& registers) -> bool {
  cpuidreg leaf{base};
  cpuidreg maxleaf{base};
  bool success = false;

  while (true) {
    auto result = get_cpuid(reader, leaf, 0);
    if (result) {
      success = true;
      registers.push_back(*result);
      if (leaf == base) {
        maxleaf = result->res.eax;
        if ((maxleaf & 0xFFFFFF00) != (leaf & 0xFFFFFF00)) return true;
      }

      if (leaf >= maxleaf) return success;
      leaf++;
    }
  }
}

auto dump_cpu_generic(cpuidreader& reader)
    -> std::optional<std::vector<cpuid_info>> {
  std::vector<cpuid_info> registers{};
  bool success = dump_range(reader, 0x00000000, registers);
  success = success && dump_range(reader, 0x80000000, registers);
  success = success && dump_range(reader, 0x40000000, registers);
  if (!success) return std::nullopt;
  return registers;
}

auto dump_hypervisor(cpuidreader& reader, std::vector<cpuid_info>& registers)
    -> bool {
  bool hyp{};
  for (auto regs : registers) {
    if (regs.req.eax == 1) {
      hyp = (regs.res.ecx & 0x80000000) != 0;
      break;
    }
  }
  if (!hyp) return true;

  return dump_range(reader, 0x40000000, registers);
}

}  // namespace rjcp::cpuid
