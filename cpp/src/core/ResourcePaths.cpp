#include "core/ResourcePaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace {
bool hasLabelImgResources(const QString &candidate) {
    QDir dir(candidate);
    return QFileInfo::exists(dir.filePath(QStringLiteral("resources/strings/strings.properties"))) &&
           QFileInfo::exists(dir.filePath(QStringLiteral("data/predefined_classes.txt")));
}
}

QString ResourcePaths::root() {
    QStringList candidates;
    const QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty()) {
        candidates << appDir << QDir(appDir).filePath(QStringLiteral(".."));
    }
#ifdef LABELIMG_REPO_ROOT
    candidates << QString::fromUtf8(LABELIMG_REPO_ROOT);
#endif
    candidates << QDir::currentPath();

    for (const QString &candidate : candidates) {
        const QString cleanPath = QDir(candidate).cleanPath(candidate);
        if (hasLabelImgResources(cleanPath)) {
            return QFileInfo(cleanPath).absoluteFilePath();
        }
    }

    return QFileInfo(appDir.isEmpty() ? QDir::currentPath() : appDir).absoluteFilePath();
}

QString ResourcePaths::filePath(const QString &relativePath) {
    return QDir(root()).filePath(relativePath);
}
