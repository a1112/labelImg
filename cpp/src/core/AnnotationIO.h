#pragma once

#include "core/Shape.h"

#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

struct AnnotationDocument {
    QString imagePath;
    QSize imageSize;
    int depth = 3;
    bool verified = false;
    QVector<Shape> shapes;
};

class AnnotationIO {
public:
    static bool loadPascalVoc(const QString &path, AnnotationDocument *document);
    static bool savePascalVoc(const QString &path, const AnnotationDocument &document);

    static bool loadYolo(const QString &path, const QSize &imageSize, AnnotationDocument *document, const QString &classListPath = {});
    static bool saveYolo(const QString &path, const AnnotationDocument &document, QStringList classList = {});

    static bool loadCreateMl(const QString &path, const QString &imagePath, AnnotationDocument *document);
    static bool saveCreateMl(const QString &path, const AnnotationDocument &document);

private:
    static QRect boundedRectFromShape(const Shape &shape);
};
