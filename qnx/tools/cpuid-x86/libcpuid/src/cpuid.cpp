#include "cpuid/cpuid.h"

#include <array>
#include <optional>
#include <string>

#include "dump_amd.h"
#include "dump_generic.h"
#include "dump_intel.h"

namespace rjcp::cpuid {

namespace {

/// @brief Decode the given register (0, 0) that contains brand information.
///
/// @param reg The results of querying the leaf 0 containing the brand
/// information.
///
/// @return The brand string contained in the register.
auto get_cpu_brand(cpuid_info& reg) -> std::string {
  if (reg.req.eax != 0 || reg.req.ecx != 0) {
    // Ensure the correct leaf is given..
    return std::string{};
  }

  std::array<char, 13> brand{};
  brand[0] = static_cast<char>(reg.res.ebx & 0xFF);
  brand[1] = static_cast<char>((reg.res.ebx >> 8) & 0xFF);
  brand[2] = static_cast<char>((reg.res.ebx >> 16) & 0xFF);
  brand[3] = static_cast<char>((reg.res.ebx >> 24) & 0xFF);
  brand[4] = static_cast<char>(reg.res.edx & 0xFF);
  brand[5] = static_cast<char>((reg.res.edx >> 8) & 0xFF);
  brand[6] = static_cast<char>((reg.res.edx >> 16) & 0xFF);
  brand[7] = static_cast<char>((reg.res.edx >> 24) & 0xFF);
  brand[8] = static_cast<char>(reg.res.ecx & 0xFF);
  brand[9] = static_cast<char>((reg.res.ecx >> 8) & 0xFF);
  brand[10] = static_cast<char>((reg.res.ecx >> 16) & 0xFF);
  brand[11] = static_cast<char>((reg.res.ecx >> 24) & 0xFF);
  brand[12] = 0;

  return std::string{brand.data()};
}

}  // namespace

auto cpuid_dump(cpuidreader& reader, unsigned int core)
    -> std::optional<std::vector<cpuid_info>> {
  if (!reader.has_cpuid()) return std::nullopt;

  auto ctx = reader.enable_core(core);
  if (!ctx || !*ctx) return std::nullopt;

  auto leaf0 = get_cpuid(reader, 0, 0);
  if (!leaf0) return std::nullopt;
  auto brand = get_cpu_brand(*leaf0);
  if (brand == "GenuineIntel") {
    return dump_cpu_intel(reader);
  } else if (brand == "AuthenticAMD" || brand == "AMDisbetter!") {
    return dump_cpu_amd(reader);
  } else {
    return dump_cpu_generic(reader);
  }

  // clang-format off
  //
  // Other CPUs:
  // - AMD:      ebx=68747541 edx=69746e65 ecx=444d4163 (Auth enti cAMD)
  // - Centaur:  ebx=746e6543 edx=48727561 ecx=736c7561 (Cent aurH auls)
  // - Cyrix:    ebx=69727943 edx=736e4978 ecx=64616574 (Cyri xIns tead)
  // - Intel     ebx=756e6547 edx=49656e69 ecx=6c65746e (Genu ineI ntel)
  // - TM1:      ebx=6e617254 edx=74656d73 ecx=55504361 (Tran smet aCPU)
  // - TM2:      ebx=756e6547 edx=54656e69 ecx=3638784d (Genu ineT Mx86)
  // - NSC:      ebx=646f6547 edx=79622065 ecx=43534e20 (Geod e by  NSC)
  // - NEXGEN:   ebx=4778654e edx=72446e65 ecx=6e657669 (NexG enDr iven)
  // - RISE:     ebx=65736952 edx=65736952 ecx=65736952 (Rise Rise Rise)
  // - SIS:      ebx=20536953 edx=20536953 ecx=20536953 (SiS  SiS  Sis )
  // - UMX:      ebx=20434d55 edx=20434d55 ecx=20434d55 (UMC  UMC  UMC )
  // - VIA:      ebx=20414956 edx=20414956 ecx=20414956 (VIA  VIA  VIA )
  // - VORTEX:   ebx=74726f56 edx=36387865 ecx=436f5320 (Vort ex86  SoC)
  // - SHANGHAI: ebx=68532020 edx=68676e61 ecx=20206961 (  Sh angh ai  )
  //
  // See [NetBSD]
  //
  // [NetBSD]: https://github.com/NetBSD/src/blob/0fc285f/external/gpl3/gcc/dist/gcc/config/i386/pentium.md
  //
  // clang-format on
}

}  // namespace rjcp::cpuid
