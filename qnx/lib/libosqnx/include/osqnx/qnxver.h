#ifdef __QNX__
#include <sys/neutrino.h>

#if __QNX__ >= 800

#define QNXVER __QNX__

#else

#include <sys/nto_version.h>
#define QNXVER _NTO_VERSION

#endif

namespace os::qnx {

constexpr int qnxversion = QNXVER;

}

#endif
