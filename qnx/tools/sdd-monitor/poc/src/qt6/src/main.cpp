#include <QApplication>

#include "sdd/main_presenter.h"
#include "qt_main_view.h"

int main(int argc, char* argv[]) {
  QApplication app{argc, argv};

  // Declaration order sets destruction order: the presenter joins its worker
  // before the view goes.
  sdd::qt_main_view view;
  sdd::main_presenter presenter{view};
  view.set_listener(presenter);
  view.show();
  presenter.start();

  return app.exec();
}
