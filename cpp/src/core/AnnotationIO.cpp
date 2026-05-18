#include "core/AnnotationIO.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringConverter>
#include <QTextStream>

QRect AnnotationIO::boundedRectFromShape(const Shape &shape) {
    QRect rect = shape.boundingRect().toAlignedRect();
    if (rect.left() < 1) rect.setLeft(1);
    if (rect.top() < 1) rect.setTop(1);
    return rect;
}

bool AnnotationIO::savePascalVoc(const QString &path, const AnnotationDocument &document) {
    QDomDocument xml;
    QDomElement root = xml.createElement("annotation");
    if (document.verified) {
        root.setAttribute("verified", "yes");
    }
    xml.appendChild(root);

    QFileInfo imageInfo(document.imagePath);
    auto appendText = [&](QDomElement parent, const QString &name, const QString &text) {
        QDomElement element = xml.createElement(name);
        element.appendChild(xml.createTextNode(text));
        parent.appendChild(element);
        return element;
    };

    appendText(root, "folder", imageInfo.dir().dirName());
    appendText(root, "filename", imageInfo.fileName());
    appendText(root, "path", document.imagePath);
    QDomElement source = xml.createElement("source");
    root.appendChild(source);
    appendText(source, "database", "Unknown");

    QDomElement size = xml.createElement("size");
    root.appendChild(size);
    appendText(size, "width", QString::number(document.imageSize.width()));
    appendText(size, "height", QString::number(document.imageSize.height()));
    appendText(size, "depth", QString::number(document.depth));
    appendText(root, "segmented", "0");

    for (const Shape &shape : document.shapes) {
        QRect rect = boundedRectFromShape(shape);
        QDomElement object = xml.createElement("object");
        root.appendChild(object);
        appendText(object, "name", shape.label);
        appendText(object, "pose", "Unspecified");
        bool truncated = rect.left() == 1 || rect.top() == 1 ||
                         rect.right() == document.imageSize.width() ||
                         rect.bottom() == document.imageSize.height();
        appendText(object, "truncated", truncated ? "1" : "0");
        appendText(object, "difficult", shape.difficult ? "1" : "0");
        QDomElement box = xml.createElement("bndbox");
        object.appendChild(box);
        appendText(box, "xmin", QString::number(rect.left()));
        appendText(box, "ymin", QString::number(rect.top()));
        appendText(box, "xmax", QString::number(rect.right()));
        appendText(box, "ymax", QString::number(rect.bottom()));
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    xml.save(stream, 1);
    return true;
}

bool AnnotationIO::loadPascalVoc(const QString &path, AnnotationDocument *document) {
    QFile file(path);
    if (!document || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QDomDocument xml;
    if (!xml.setContent(&file)) {
        return false;
    }

    QDomElement root = xml.documentElement();
    document->verified = root.attribute("verified") == "yes";
    document->imagePath = root.firstChildElement("path").text();
    int width = root.firstChildElement("size").firstChildElement("width").text().toInt();
    int height = root.firstChildElement("size").firstChildElement("height").text().toInt();
    document->depth = root.firstChildElement("size").firstChildElement("depth").text().toInt();
    document->imageSize = QSize(width, height);
    document->shapes.clear();

    QDomNodeList objects = root.elementsByTagName("object");
    for (int i = 0; i < objects.size(); ++i) {
        QDomElement object = objects.at(i).toElement();
        QString label = object.firstChildElement("name").text();
        bool difficult = object.firstChildElement("difficult").text().toInt() != 0;
        QDomElement box = object.firstChildElement("bndbox");
        int xMin = box.firstChildElement("xmin").text().toInt();
        int yMin = box.firstChildElement("ymin").text().toInt();
        int xMax = box.firstChildElement("xmax").text().toInt();
        int yMax = box.firstChildElement("ymax").text().toInt();
        document->shapes.push_back(Shape::fromRect(label, QRectF(xMin, yMin, xMax - xMin + 1, yMax - yMin + 1), difficult));
    }
    return true;
}

bool AnnotationIO::saveYolo(const QString &path, const AnnotationDocument &document, QStringList classList) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    for (const Shape &shape : document.shapes) {
        if (!classList.contains(shape.label)) {
            classList.append(shape.label);
        }
        int classIndex = classList.indexOf(shape.label);
        QRectF rect = shape.boundingRect();
        double xCenter = rect.center().x() / document.imageSize.width();
        double yCenter = rect.center().y() / document.imageSize.height();
        double width = rect.width() / document.imageSize.width();
        double height = rect.height() / document.imageSize.height();
        stream << classIndex << ' '
               << QString::number(xCenter, 'f', 6) << ' '
               << QString::number(yCenter, 'f', 6) << ' '
               << QString::number(width, 'f', 6) << ' '
               << QString::number(height, 'f', 6) << '\n';
    }

    QFile classFile(QFileInfo(path).dir().filePath("classes.txt"));
    if (!classFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream classStream(&classFile);
    classStream.setEncoding(QStringConverter::Utf8);
    for (const QString &className : classList) {
        classStream << className << '\n';
    }
    return true;
}

bool AnnotationIO::loadYolo(const QString &path, const QSize &imageSize, AnnotationDocument *document, const QString &classListPath) {
    if (!document) {
        return false;
    }
    QFile classFile(classListPath.isEmpty() ? QFileInfo(path).dir().filePath("classes.txt") : classListPath);
    QFile file(path);
    if (!classFile.open(QIODevice::ReadOnly | QIODevice::Text) || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QStringList classes = QString::fromUtf8(classFile.readAll()).split('\n', Qt::SkipEmptyParts);
    document->imageSize = imageSize;
    document->shapes.clear();
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QStringList parts = stream.readLine().split(' ', Qt::SkipEmptyParts);
        if (parts.size() != 5) {
            continue;
        }
        int classIndex = parts[0].toInt();
        double xCenter = parts[1].toDouble();
        double yCenter = parts[2].toDouble();
        double width = parts[3].toDouble();
        double height = parts[4].toDouble();
        QRectF rect((xCenter - width / 2.0) * imageSize.width(),
                    (yCenter - height / 2.0) * imageSize.height(),
                    width * imageSize.width(),
                    height * imageSize.height());
        document->shapes.push_back(Shape::fromRect(classes.value(classIndex), rect, false));
    }
    return true;
}

bool AnnotationIO::saveCreateMl(const QString &path, const AnnotationDocument &document) {
    QJsonArray root;
    QFile existing(path);
    if (existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
        root = QJsonDocument::fromJson(existing.readAll()).array();
    }

    QFileInfo imageInfo(document.imagePath);
    QJsonObject imageObject;
    imageObject["image"] = imageInfo.fileName();
    imageObject["verified"] = document.verified;
    QJsonArray annotations;
    for (const Shape &shape : document.shapes) {
        QRectF rect = shape.boundingRect();
        QJsonObject coordinates;
        coordinates["x"] = rect.center().x();
        coordinates["y"] = rect.center().y();
        coordinates["width"] = rect.width();
        coordinates["height"] = rect.height();
        QJsonObject annotation;
        annotation["label"] = shape.label;
        annotation["coordinates"] = coordinates;
        annotations.append(annotation);
    }
    imageObject["annotations"] = annotations;

    bool replaced = false;
    for (int i = 0; i < root.size(); ++i) {
        if (root[i].toObject().value("image").toString() == imageInfo.fileName()) {
            root.replace(i, imageObject);
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        root.append(imageObject);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return true;
}

bool AnnotationIO::loadCreateMl(const QString &path, const QString &imagePath, AnnotationDocument *document) {
    QFile file(path);
    if (!document || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QJsonArray root = QJsonDocument::fromJson(file.readAll()).array();
    QString fileName = QFileInfo(imagePath).fileName();
    document->imagePath = imagePath;
    document->shapes.clear();
    for (const QJsonValue &value : root) {
        QJsonObject imageObject = value.toObject();
        if (imageObject.value("image").toString() != fileName) {
            continue;
        }
        document->verified = imageObject.value("verified").toBool(false);
        for (const QJsonValue &annotationValue : imageObject.value("annotations").toArray()) {
            QJsonObject annotation = annotationValue.toObject();
            QJsonObject coordinates = annotation.value("coordinates").toObject();
            double width = coordinates.value("width").toDouble();
            double height = coordinates.value("height").toDouble();
            QRectF rect(coordinates.value("x").toDouble() - width / 2.0,
                        coordinates.value("y").toDouble() - height / 2.0,
                        width,
                        height);
            document->shapes.push_back(Shape::fromRect(annotation.value("label").toString(), rect, true));
        }
        return true;
    }
    return true;
}
