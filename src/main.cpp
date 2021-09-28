#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("EKA2L1");
    QCoreApplication::setApplicationName("EKA2L1");

    MainWindow *window = new MainWindow();
    window->show();

    const int code = a.exec();

    delete window;
    return code;
}
