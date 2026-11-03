#pragma once

#include <QMainWindow>
#include <optional>

#include "sdd/computer.h"
#include "sdd/i_main_view.h"
#include "sdd/i_view_listener.h"
#include "sdd/qt/qt_computer.h"
#include "computer_list_model.h"

class QLineEdit;
class QListView;
class QTreeWidget;

namespace sdd {

class qt_main_view : public QMainWindow, public i_main_view {
  Q_OBJECT

 public:
  explicit qt_main_view(QWidget* parent = nullptr);

  void set_listener(i_view_listener& listener);

  // Any thread. Emits computer_shown, which is delivered on the UI thread.
  void show_computer(const computer& c) override;

 signals:
  void computer_shown(const sdd::qt_computer& c);

 private slots:
  void on_computer_shown(const sdd::qt_computer& c);
  void on_current_changed(
      const QModelIndex& current, const QModelIndex& previous);

 private:
  void build_ui();
  void show_details(const sdd::qt_computer& c);

  computer_list_model* model_ = nullptr;
  i_view_listener* listener_ = nullptr;
  std::optional<computer_id> selected_;
  QListView* list_ = nullptr;
  QLineEdit* name_edit_ = nullptr;
  QLineEdit* os_edit_ = nullptr;
  QTreeWidget* interfaces_ = nullptr;
};

}  // namespace sdd
