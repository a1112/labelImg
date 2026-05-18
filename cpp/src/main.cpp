#include "ui/MainWindow.h"

#include "core/ResourcePaths.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("labelImgCpp");
    QApplication::setOrganizationName("labelImg");
    QApplication::setWindowIcon(QIcon(ResourcePaths::filePath(QStringLiteral("resources/icons/app-cpp.png"))));

    MainWindow window;
    window.show();
    return app.exec();
}
