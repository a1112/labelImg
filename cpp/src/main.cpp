#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("labelImgCpp");
    QApplication::setOrganizationName("labelImg");

    MainWindow window;
    window.show();
    return app.exec();
}
