#include "qt_main_view.h"

#include <QAction>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace sdd {
namespace {

constexpr int list_width = 180;

qt_computer to_qt(const computer& c) {
  qt_computer out;
  out.id = c.id;
  out.name = QString::fromStdString(c.name);
  out.os = QString::fromStdString(c.os);
  for (const interface_info& i : c.interfaces) {
    qt_interface qi;
    qi.name = QString::fromStdString(i.name);
    qi.mac = QString::fromStdString(i.mac);
    for (const std::string& ip : i.ips) {
      qi.ips.append(QString::fromStdString(ip));
    }
    out.interfaces.append(qi);
  }
  return out;
}

}  // namespace

qt_main_view::qt_main_view(QWidget* parent) : QMainWindow{parent} {
  qRegisterMetaType<sdd::qt_computer>();
  connect(this, &qt_main_view::computer_shown, this,
      &qt_main_view::on_computer_shown);
  build_ui();
}

void qt_main_view::set_listener(i_view_listener& listener) {
  listener_ = &listener;
}

void qt_main_view::show_computer(const computer& c) {
  emit computer_shown(to_qt(c));
}

void qt_main_view::on_computer_shown(const qt_computer& c) {
  model_->upsert(c);
  if (selected_ && *selected_ == c.id) {
    show_details(c);
  }
}

void qt_main_view::on_current_changed(
    const QModelIndex& current, const QModelIndex&) {
  if (!current.isValid()) {
    selected_.reset();
    show_details({});
    if (listener_) {
      listener_->on_selected(std::nullopt);
    }
    return;
  }
  const computer_id id =
      current.data(computer_list_model::id_role).toULongLong();
  selected_ = id;
  if (const qt_computer* c = model_->find(id)) {
    show_details(*c);
  }
  if (listener_) {
    listener_->on_selected(id);
  }
}

void qt_main_view::build_ui() {
  setWindowTitle(tr("SDD Monitor"));

  auto* file_menu = menuBar()->addMenu(tr("&File"));
  auto* exit_action = file_menu->addAction(tr("E&xit"));
  connect(exit_action, &QAction::triggered, this, &QWidget::close);

  model_ = new computer_list_model(this);

  list_ = new QListView(this);
  list_->setModel(model_);
  list_->setFixedWidth(list_width);
  list_->setIconSize(QSize{32, 32});
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  connect(list_->selectionModel(), &QItemSelectionModel::currentChanged, this,
      &qt_main_view::on_current_changed);

  name_edit_ = new QLineEdit(this);
  os_edit_ = new QLineEdit(this);
  name_edit_->setReadOnly(true);
  os_edit_->setReadOnly(true);

  interfaces_ = new QTreeWidget(this);
  interfaces_->setHeaderLabels({tr("Interface"), tr("Value")});

  auto* form = new QFormLayout;
  form->addRow(tr("Name:"), name_edit_);
  form->addRow(tr("Operating system:"), os_edit_);

  auto* details = new QWidget(this);
  auto* details_layout = new QVBoxLayout(details);
  details_layout->addLayout(form);
  details_layout->addWidget(new QLabel(tr("Interfaces:"), this));
  details_layout->addWidget(interfaces_, 1);

  auto* central = new QWidget(this);
  auto* layout = new QHBoxLayout(central);
  layout->addWidget(list_);
  layout->addWidget(details, 1);
  setCentralWidget(central);
  resize(720, 480);
}

void qt_main_view::show_details(const sdd::qt_computer& c) {
  name_edit_->setText(c.name);
  os_edit_->setText(c.os);
  interfaces_->clear();
  for (const qt_interface& i : c.interfaces) {
    auto* item = new QTreeWidgetItem(interfaces_, QStringList{i.name});
    new QTreeWidgetItem(item, QStringList{tr("MAC"), i.mac});
    for (const QString& ip : i.ips) {
      new QTreeWidgetItem(item, QStringList{tr("IP"), ip});
    }
    item->setExpanded(true);
  }
}

}  // namespace sdd
