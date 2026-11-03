#pragma once

#include <QAbstractListModel>
#include <QIcon>

#include "sdd/qt/qt_computer.h"

namespace sdd {

class computer_list_model : public QAbstractListModel {
  Q_OBJECT

 public:
  static constexpr int id_role = Qt::UserRole + 1;

  explicit computer_list_model(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(
      const QModelIndex& index, int role = Qt::DisplayRole) const override;

  // Inserts the computer, or replaces the existing row with the same id.
  void upsert(const qt_computer& c);
  const qt_computer* find(computer_id id) const;

 private:
  QList<qt_computer> computers_;
  QIcon icon_;
};

}  // namespace sdd
