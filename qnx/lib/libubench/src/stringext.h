#ifndef UBENCH_STRINGEXT_H
#define UBENCH_STRINGEXT_H

#include "config.h"

#include <cstring>

#if HAVE_STRLCPY

#if HAVE_INCLUDE_BSD_STRING_H
// Get strlcpy() if available here (e.g. on Linux).
#include <bsd/string.h>
#endif

#else

// For systems that don't have an implementation of strlcpy, we provide our own.

/// @brief Copy src to string dst of size siz.
///
/// At most siz-1 characters will be copied.  Always NUL terminates (unless siz
/// == 0).
///
/// Note: this function is safe if only src is the same size or larger than dst.
/// If src is not NUL-terminated, out of bounds access can occur in src while
/// copying to dst. Use with std::string.c_str(), but not with
/// std::string_view.c_str().
///
/// @param dst The destination string to copy to.
///
/// @param src The source string to copy from.
///
/// @param size The maximum size available in dst.
///
/// @return The number of bytes strlen(src). If return value >= size, truncation
/// occurred.
size_t strlcpy(char *dst, const char *src, size_t size);

#endif

#endif
