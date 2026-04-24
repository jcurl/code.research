#ifndef CPUID_CPUIDREADER_XML_H
#define CPUID_CPUIDREADER_XML_H

#include <string_view>

#include "cpuid/cpuidreader_cache.h"

namespace rjcp::cpuid {

/// @brief A CPUID reader that reads from an XML file.
class cpuidreader_xml : public cpuidreader_cache {
 public:
  /// @brief Construct a cpuidreader_xml instance from an XML file.
  ///
  /// @param file_name The name of the XML file to read from.
  cpuidreader_xml(std::string_view file_name);

  cpuidreader_xml(const cpuidreader_xml&) = delete;
  cpuidreader_xml(cpuidreader_xml&&) = default;
  auto operator=(const cpuidreader_xml&) -> cpuidreader_xml& = delete;
  auto operator=(cpuidreader_xml&&) -> cpuidreader_xml& = default;
  ~cpuidreader_xml() = default;
};

}  // namespace rjcp::cpuid

#endif
