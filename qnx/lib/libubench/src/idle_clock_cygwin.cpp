#include <Windows.h>

#include "ubench/clock.h"

namespace ubench::chrono {

auto idle_clock::now() noexcept -> time_point {
  FILETIME idleTime{};
  BOOL result = GetSystemTimes(&idleTime, nullptr, nullptr);
  if (result == 0) return time_point{duration(0)};

  // https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
  //
  // It is not recommended that you add and subtract values from the FILETIME
  // structure to obtain relative times. Instead, you should copy the low- and
  // high-order parts of the file time to a ULARGE_INTEGER structure, perform
  // 64-bit arithmetic on the QuadPart member, and copy the LowPart and
  // HighPart members into the FILETIME structure.
  //
  // Do not cast a pointer to a FILETIME structure to either a ULARGE_INTEGER*
  // or __int64* value because it can cause alignment faults on 64-bit
  // Windows.
  ULARGE_INTEGER iTime{};
  iTime.LowPart = idleTime.dwLowDateTime;
  iTime.HighPart = idleTime.dwHighDateTime;

  // iTime.QuadPart is in units of 100ns intervals
  return time_point{std::chrono::duration_cast<duration>(
      std::chrono::nanoseconds(iTime.QuadPart * 100))};
}

auto idle_clock::is_available() noexcept -> bool { return true; }

auto idle_clock::type() noexcept -> idle_clock_type {
  return idle_clock_type::windows;
}

}  // namespace ubench::chrono