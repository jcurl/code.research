#ifndef UBENCH_MEASUREMENT_PRINT_H
#define UBENCH_MEASUREMENT_PRINT_H

#include <initializer_list>
#include <ostream>
#include <string>
#include <vector>

namespace ubench::measure {

class table;

enum class alignment {
  left,    //< Align column to the left.
  centre,  //< Align column to the centre.
  right,   //< Align column to the right.
};

}  // namespace ubench::measure

/// @brief Stream operator for printing a table.
///
/// @param os the stream to write to.
///
/// @param table the table to write.
///
/// @return the stream operator for chaining writes.
auto operator<<(std::ostream& os, const ubench::measure::table& table)
    -> std::ostream&;

namespace ubench::measure {

/// @brief A table representation containing data that can be later printed.
///
/// The table is a matrix of strings. Program the headers by calling
/// add_column() first. Then write each row.
///
/// Using the stream operator, the table will be written. The current
/// implementation provides a mark-down like format. It might be in the future
/// that other formats can be added (e.g. JSON, MyST list tables, etc.).
class table {
 public:
  table() = default;
  table(const table&) = default;
  auto operator=(const table&) -> table& = default;
  table(table&&) = default;
  auto operator=(table&&) -> table& = default;
  ~table() = default;

  /// @brief Add a column to the table.
  ///
  /// Adds an extra column to the table. No rows must have been added prior with
  /// add_line().
  ///
  /// @param col_name the name of the column.
  ///
  /// @return true if the column was added.
  auto add_column(
      const std::string& col_name, alignment align = alignment::left) -> bool;

  /// @brief Add a new row to the table.
  ///
  /// Once adding a line, it is no longer possible to add rows.
  ///
  /// @param line a list of cells to add to the line. The number of cells must
  /// match the number of columns.
  ///
  /// @return true if the row was added.
  auto add_line(std::initializer_list<std::string> line) -> bool;

  /// @brief The number of rows added to the table.
  ///
  /// @return the number of rows added to the table.
  [[nodiscard]] auto size() const -> unsigned long { return table_.size(); }

 private:
  std::vector<std::string> hdr_{};
  std::vector<std::vector<std::string>> table_{};

  struct colprop {
    unsigned int width;
    alignment align;
  };
  std::vector<colprop> colprop_{};

  auto update_cols(std::vector<std::string>& line) -> void;
  auto md_print_row(std::ostream& os, const std::vector<std::string>& row,
      const std::vector<colprop>& prop) const -> void;
  auto md_print_sep(std::ostream& os, const std::vector<colprop>& prop,
      char fill) const -> void;
  friend auto ::operator<<(std::ostream& os, const table& table)
      -> std::ostream&;
};

}  // namespace ubench::measure

#endif
