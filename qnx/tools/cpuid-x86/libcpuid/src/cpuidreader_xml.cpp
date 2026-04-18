#include "config.h"

#include "cpuid/cpuidreader_xml.h"

#if !HAVE_LIBXML2

// ----------------------------------------------------------------------------
// LibXml2 not available, default implementation. Do nothing.

namespace rjcp::cpuid {

cpuidreader_xml::cpuidreader_xml([[maybe_unused]] std::string_view file_name) { }

}  // namespace rjcp::cpuid

#else

// ----------------------------------------------------------------------------
// LibXml2 available

#include <cstdint>
#include <optional>
#include <string>

#include "cpuid/cpuid.h"
#include "libxml/parser.h"
#include "libxml/xmlstring.h"

namespace rjcp::cpuid {

namespace {

/// @brief Convert an xmlChar to an integer given the base.
///
/// @param value the string read from the buffer. On exit, the pointer *value is
/// advanced to where the string stopped parsing.
///
/// @param base the base the string is represented as, usually 10 or 16.
///
/// @return the integer value, or std::nullopt.
auto xmlCharToInt(const xmlChar** value, int base)
    -> std::optional<std::uint32_t> {
  if (value == nullptr) return std::nullopt;
  if (*value == nullptr) return std::nullopt;
  if (**value == 0) return std::nullopt;
  char* endptr = nullptr;

  auto result =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      std::strtoul(reinterpret_cast<const char*>(*value), &endptr, base);

  // Value out of bounds.
  if (result > 0xFFFFFFFF) return std::nullopt;

  // No value interpreted.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (reinterpret_cast<const char*>(*value) == endptr) return std::nullopt;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  *value = reinterpret_cast<const xmlChar*>(endptr);
  return result;
}

/// @brief Convert an attribute of the current node to an integer given the
/// base.
///
/// @param node the node to read the attribute from.
///
/// @param attr the attribute to read.
///
/// @param base the base the string is represented as, usually 10 or 16.
///
/// @return the integer value, or std::nullopt.
auto xmlGetPropInt(xmlNodePtr node, std::string attr, int base)
    -> std::optional<std::uint32_t> {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto a = reinterpret_cast<const xmlChar*>(attr.c_str());

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto v = reinterpret_cast<const xmlChar*>(xmlGetProp(node, a));

  auto endptr = &v;
  auto result = xmlCharToInt(endptr, base);
  if (!result.has_value() || **endptr != 0) return std::nullopt;
  return result;
}

/// @brief Convert an xmlChar to an integer given the base. Ensures the string
/// ends with either `\0` or a comma ','.
///
/// @param value the string read from the buffer. On exit, the pointer *value is
/// advanced to where the string stopped parsing, and after the comma if found.
///
/// @param base the base the string is represented as, usually 10 or 16.
///
/// @return the integer value, or std::nullopt.
auto xmlGetIntField(const xmlChar** value, int base)
    -> std::optional<std::uint32_t> {
  // Skip over leading spaces.
  while (**value == ' ') {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    (*value)++;
  }

  auto result = xmlCharToInt(value, base);
  if (!result.has_value()) return std::nullopt;

  // Skip over trailing spaces.
  if (**value != 0) {
    while (**value == ' ') {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      (*value)++;
    }
    if (**value != ',' && **value != 0) {
      return std::nullopt;
    }
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  if (**value != 0) (*value)++;
  return result;
}

/// @brief manages an XML document.
///
/// Abstracts the XML document away from the implementation of the
/// xpuidreader_xml, that the header file does not need any references to
/// libxml2.
class xmldoc {
 public:
  /// @brief Load the XML file as a document in memory.
  ///
  /// @param file_name the file to load
  xmldoc(std::string file_name) {
    if (!initialise(std::move(file_name))) free();
  }

  xmldoc(const xmldoc&) = delete;
  auto operator=(const xmldoc&) -> xmldoc& = delete;

  xmldoc(xmldoc&&) = default;
  auto operator=(xmldoc&&) -> xmldoc& = default;

  ~xmldoc() { free(); }

  /// @brief Check if the document is loaded.
  ///
  /// @return true if the document is loaded, false otherwise.
  auto is_loaded() -> bool { return doc_ != nullptr; }

  /// @brief Parse through the XML at the current node, looking for the start of
  /// the next core.
  ///
  /// @return the core number found, or std::nullopt of no further nodes found.
  auto next_core() -> std::optional<unsigned int> {
    while (proc_node_ != nullptr) {
      if (proc_node_->type == XML_ELEMENT_NODE &&
          xmlStrEqual(proc_node_->name,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
              reinterpret_cast<const xmlChar*>("processor"))) {
        reg_node_ = proc_node_->xmlChildrenNode;

        proc_node_id_++;
        proc_node_ = proc_node_->next;
        return proc_node_id_ - 1;
      }
      proc_node_ = proc_node_->next;
    }
    return std::nullopt;
  }

  /// @brief Parse through the XML returning the next CPUID information element.
  ///
  /// @return the cpuid_info element found, or std::nullopt of no further nodes
  /// found.
  auto next_entry() -> std::optional<cpuid_info> {
    while (reg_node_ != nullptr) {
      if (reg_node_->type == XML_ELEMENT_NODE &&
          xmlStrEqual(reg_node_->name,
              // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
              reinterpret_cast<const xmlChar*>("register"))) {
        // Get the input parameters.
        auto eax = xmlGetPropInt(reg_node_, "eax", 16);
        auto ecx = xmlGetPropInt(reg_node_, "ecx", 16);
        if (!eax.has_value() || !ecx.has_value()) {
          reg_node_ = reg_node_->next;
          continue;
        }

        // Get the output parameters. This is a list EAX,EBX,ECX,EDX. We iterate
        // over the list looking for separators or the end.
        xmlChar* key{};
        key = xmlNodeListGetString(doc_, reg_node_->xmlChildrenNode, 1);

        const xmlChar* pos = key;
        auto veax = xmlGetIntField(&pos, 16);
        auto vebx = xmlGetIntField(&pos, 16);
        auto vecx = xmlGetIntField(&pos, 16);
        auto vedx = xmlGetIntField(&pos, 16);
        bool success = *pos == 0;
        xmlFree(key);

        if (!veax.has_value() || !vebx.has_value() || !vecx.has_value() ||
            !vedx.has_value() || !success) {
          reg_node_ = reg_node_->next;
          continue;
        }

        // Now return the value and advance to the next.
        cpuid_info info{{*eax, *ecx}, {*veax, *vebx, *vecx, *vedx}};
        reg_node_ = reg_node_->next;
        return info;
      }
      reg_node_ = reg_node_->next;
    }
    return std::nullopt;
  }

 private:
  xmlDocPtr doc_{};
  unsigned int proc_node_id_{};
  xmlNodePtr proc_node_{};
  xmlNodePtr reg_node_{};

  auto initialise(std::string file_name) -> bool {
    doc_ = xmlReadFile(file_name.c_str(), nullptr,
        XML_PARSE_NONET | XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
    if (doc_ == nullptr) return false;

    xmlNodePtr root_node = xmlDocGetRootElement(doc_);
    if (root_node == nullptr) return false;

    if (!xmlStrEqual(root_node->name,
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<const xmlChar*>("cpuid")))
      return false;

    proc_node_id_ = 0;
    proc_node_ = root_node->xmlChildrenNode;
    return true;
  }

  auto free() -> void {
    if (doc_ != nullptr) {
      xmlFreeDoc(doc_);
      doc_ = nullptr;
    }
  }
};

}  // namespace

cpuidreader_xml::cpuidreader_xml(std::string_view file_name) {
  xmldoc doc{std::string{file_name}};
  if (!doc.is_loaded()) return;

  auto core = doc.next_core();
  while (core.has_value()) {
    set_cores(core.value() + 1);
    auto info = doc.next_entry();
    while (info) {
      add_cpuid(core.value(), info->req.eax, info->req.ecx, info->res);
      info = doc.next_entry();
    }
    core = doc.next_core();
  }
}

}  // namespace rjcp::cpuid

#endif
