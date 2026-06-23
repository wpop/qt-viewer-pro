#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  // Configure application settings storage.
  QApplication::setOrganizationName("wpop");
  QApplication::setApplicationName("Qt Viewer");

  MainWindow window;
  window.show();

  return app.exec();
}
