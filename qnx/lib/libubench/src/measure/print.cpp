#include "ubench/measure/print.h"

#include <algorithm>
#include <iomanip>
#include <ios>
#include <iostream>

namespace ubench::measure {

auto table::md_print_row(std::ostream& os, const std::vector<std::string>& row,
    const std::vector<ubench::measure::table::colprop>& prop) const -> void {
  os << "| ";
  for (size_t p = 0; p < row.size(); p++) {
    if (p) os << " | ";
    switch (prop[p].align) {
      case alignment::left:
        os << std::setw(static_cast<int>(prop[p].width)) << std::left << row[p];
        break;
      case alignment::centre: {
        int w = static_cast<int>(prop[p].width - row[p].length()) / 2;
        os << std::left << std::setw(w) << "";
        os << std::left << std::setw(static_cast<int>(prop[p].width) - w)
           << row[p];
        break;
      }
      case alignment::right:
        os << std::setw(static_cast<int>(prop[p].width)) << std::right
           << row[p];
        break;
    }
  }
  os << " |";
}

auto table::md_print_sep(std::ostream& os, const std::vector<colprop>& prop,
    char fill) const -> void {
  os << "| ";
  for (size_t p = 0; p < prop.size(); p++) {
    if (p) os << " | ";
    if (fill == ' ') {
      os << std::setw(static_cast<int>(prop[p].width)) << std::setfill(' ')
         << "" << std::setfill(' ');
    } else {
      switch (prop[p].align) {
        case alignment::left:
          os << std::setw(static_cast<int>(prop[p].width)) << std::setfill(fill)
             << "" << std::setfill(' ');
          break;
        case alignment::centre:
          os << ":" << std::setw(static_cast<int>(prop[p].width - 2))
             << std::setfill(fill) << "" << std::setfill(' ') << ":";
          break;
        case alignment::right:
          os << std::setw(static_cast<int>(prop[p].width - 1))
             << std::setfill(fill) << "" << std::setfill(' ') << ":";
          break;
      }
    }
  }
  os << " |";
}

auto table::update_cols(std::vector<std::string>& line) -> void {
  if (colprop_.empty()) {
    colprop_.resize(line.size());
    int p{};
    for (const auto& cell : line) {
      colprop_[p].width = static_cast<unsigned int>(cell.length());
      p++;
    }
    return;
  }

  int p{};
  for (const auto& cell : line) {
    colprop_[p].width =
        std::max(colprop_[p].width, static_cast<unsigned int>(cell.length()));
    p++;
  }
}

auto table::add_column(const std::string& col_name, alignment align) -> bool {
  // The user has already started adding rows. Can't add more columns.
  if (!table_.empty()) return false;

  hdr_.emplace_back(col_name);
  colprop p = {
      .width = static_cast<unsigned int>(col_name.length()), .align = align};
  colprop_.emplace_back(p);
  return true;
}

auto table::add_line(std::initializer_list<std::string> line) -> bool {
  // The number of columns given for this row doesn't match.
  if (!colprop_.empty() && line.size() != colprop_.size()) return false;

  auto& l = table_.emplace_back(line);
  update_cols(l);
  return true;
}

}  // namespace ubench::measure

auto operator<<(std::ostream& os, const ubench::measure::table& table)
    -> std::ostream& {
  if (!table.hdr_.empty()) {
    table.md_print_row(os, table.hdr_, table.colprop_);
    os << std::endl;
    table.md_print_sep(os, table.colprop_, '-');
  }

  if (!table.table_.empty()) {
    bool newline = !table.hdr_.empty();

    for (const auto& row : table.table_) {
      if (newline) os << std::endl;
      table.md_print_row(os, row, table.colprop_);
      newline = true;
    }
  } else {
    if (!table.hdr_.empty()) {
      os << std::endl;
      table.md_print_sep(os, table.colprop_, ' ');
    }
  }

  return os;
}
