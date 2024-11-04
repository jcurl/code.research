#include <Windows.h>

#include "ubench/clock.h"

namespace ubench::chrono {

auto process_clock::now() noexcept -> time_point {
  FILETIME creationTime{};
  FILETIME exitTime{};
  FILETIME kernelTime{};
  FILETIME userTime{};
  HANDLE process = GetCurrentProcess();
  BOOL result = GetProcessTimes(process, &creationTime, &exitTime, &kernelTime,
                                &userTime);
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
  ULARGE_INTEGER kTime{};
  kTime.LowPart = kernelTime.dwLowDateTime;
  kTime.HighPart = kernelTime.dwHighDateTime;
  ULARGE_INTEGER uTime{};
  uTime.LowPart = userTime.dwLowDateTime;
  uTime.HighPart = userTime.dwHighDateTime;
  ULARGE_INTEGER tTime{};
  tTime.QuadPart = kTime.QuadPart + uTime.QuadPart;

  // tTime.QuadPart is in units of 100ns intervals
  return time_point{std::chrono::duration_cast<duration>(
      std::chrono::nanoseconds(tTime.QuadPart * 100))};
}

auto process_clock::is_available() noexcept -> bool { return true; }

}  // namespace ubench::chrono
