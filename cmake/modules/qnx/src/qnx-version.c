#include <sys/neutrino.h>
#include <sys/nto_version.h>

#ifndef __QNX__
# error Variable __QNX__ not found
#else
# define VALUE_TO_STRING(x) "QNX_VERSION: " #x
# define VALUE(x) VALUE_TO_STRING(x)
# if __QNX__ >= 800
#  pragma message(VALUE(__QNX__))
# else
#  pragma message(VALUE(_NTO_VERSION))
# endif
#endif

int main() { return 0; }