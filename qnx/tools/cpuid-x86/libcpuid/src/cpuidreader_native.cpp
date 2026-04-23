#include "config.h"

#include "cpuid/cpuidreader_native.h"

namespace rjcp::cpuid {

auto cpuidreader_native::has_cpuid() -> bool { return HAVE_CPUIDX86 != 0; }

}  // namespace rjcp::cpuid
