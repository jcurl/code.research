#include "ubench/string.h"

auto strlcpy(char *dst, const char *src, size_t size) -> size_t {
  // We don't want to do a strlen() then a copy, as this traverses the string
  // twice.

  char *d = dst;
  const char *s = src;
  size_t n = size;

  if (n > 0) {
    while (--n > 0) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-assignment-in-if-condition)
      if ((*d++ = *s++) == 0) break;
    }
  }

  if (n == 0) {
    // dst is smaller than src/size.
    if (size != 0) *d = '\0';

    // Get the length of src. This is UNSAFE, if the original string is not
    // bounded. But we don't modify the specification of strlcpy. Better would
    // be to use strncpy_s. Alas, that isn't supported by GCC.

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    while (*s++)
      ;
  }

  return (s - src - 1); /* count does not include NUL */
}
