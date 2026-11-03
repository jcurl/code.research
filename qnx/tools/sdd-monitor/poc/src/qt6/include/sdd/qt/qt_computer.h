#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>

#include "sdd/computer.h"

namespace sdd {

struct qt_interface {
  QString name;
  QString mac;
  QStringList ips;
};

struct qt_computer {
  computer_id id{};
  QString name;
  QString os;
  QList<qt_interface> interfaces;
};

}  // namespace sdd

Q_DECLARE_METATYPE(sdd::qt_computer)
