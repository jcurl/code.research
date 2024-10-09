#ifndef UBENCH_THREAD_PIN_H
#define UBENCH_THREAD_PIN_H

namespace ubench::thread {

/// @brief Pin the current thread to core.
///
/// @param core The core to pin the thread to
///
/// @return true if the core was pinned, false otherwise. Use errno to get the
/// error.
auto pin_core(unsigned int core) -> bool;

}  // namespace ubench::thread

#endif