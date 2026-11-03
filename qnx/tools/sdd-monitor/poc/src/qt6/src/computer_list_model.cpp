#include "computer_list_model.h"

#include <QApplication>
#include <QStyle>
#include <algorithm>

namespace sdd {

computer_list_model::computer_list_model(QObject* parent)
    : QAbstractListModel{parent},
      icon_{QApplication::style()->standardIcon(QStyle::SP_ComputerIcon)} {}

int computer_list_model::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(computers_.size());
}

QVariant computer_list_model::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() >= static_cast<int>(computers_.size())) {
    return {};
  }
  const qt_computer& c = computers_.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
      return c.name;
    case Qt::DecorationRole:
      return icon_;
    case id_role:
      return QVariant::fromValue(c.id);
    default:
      return {};
  }
}

void computer_list_model::upsert(const qt_computer& c) {
  for (qsizetype row = 0; row < computers_.size(); ++row) {
    if (computers_[row].id == c.id) {
      computers_[row] = c;
      const QModelIndex idx = index(static_cast<int>(row));
      emit dataChanged(idx, idx);
      return;
    }
  }
  const int row = static_cast<int>(computers_.size());
  beginInsertRows(QModelIndex{}, row, row);
  computers_.append(c);
  endInsertRows();
}

const qt_computer* computer_list_model::find(computer_id id) const {
  const auto it = std::find_if(computers_.begin(), computers_.end(),
                               [id](const qt_computer& c) { return c.id == id; });
  return it == computers_.end() ? nullptr : &*it;
}

}  // namespace sdd
