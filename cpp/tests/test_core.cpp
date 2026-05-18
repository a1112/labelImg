#include <QtCore>
#include <QtTest>

#include "core/AnnotationIO.h"
#include "core/LabelListModel.h"
#include "core/PerformanceMonitor.h"
#include "core/StringBundle.h"

class CoreTests : public QObject {
    Q_OBJECT

private slots:
    void stringBundleNormalizesLocales();
    void pascalVocRoundTripsChineseLabels();
    void yoloWritesClassesInEncounterOrder();
    void createMlUpdatesExistingImageEntry();
    void labelNavigationSelectsTopWhenEmptyAndWraps();
    void shapeSupportsVertexEditingAndCopy();
    void performanceMonitorReturnsCpuAndMemoryText();
};

void CoreTests::stringBundleNormalizesLocales() {
    QCOMPARE(StringBundle::normalizeLanguage("zh_CN"), QString("zh-CN"));
    QCOMPARE(StringBundle::normalizeLanguage("zh_TW.UTF-8"), QString("zh-TW"));
    QCOMPARE(StringBundle::normalizeLanguage("ja"), QString("ja-JP"));
    QCOMPARE(StringBundle::normalizeLanguage("UTF-8"), QString("en"));
}

void CoreTests::pascalVocRoundTripsChineseLabels() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AnnotationDocument doc;
    doc.imagePath = dir.filePath(QString::fromUtf8("图像.jpg"));
    doc.imageSize = QSize(1024, 768);
    doc.depth = 3;
    doc.verified = true;
    doc.shapes.push_back(Shape::fromRect(QString::fromUtf8("缺陷"), QRectF(10, 20, 90, 120), true));

    const QString xmlPath = dir.filePath(QString::fromUtf8("标注.xml"));
    QVERIFY2(AnnotationIO::savePascalVoc(xmlPath, doc), qPrintable(xmlPath));

    AnnotationDocument loaded;
    QVERIFY(AnnotationIO::loadPascalVoc(xmlPath, &loaded));
    QVERIFY(loaded.verified);
    QCOMPARE(loaded.shapes.size(), 1);
    QCOMPARE(loaded.shapes[0].label, QString::fromUtf8("缺陷"));
    QVERIFY(loaded.shapes[0].difficult);
    QCOMPARE(loaded.shapes[0].boundingRect().toRect(), QRect(10, 20, 90, 120));
}

void CoreTests::yoloWritesClassesInEncounterOrder() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AnnotationDocument doc;
    doc.imagePath = dir.filePath("image.jpg");
    doc.imageSize = QSize(100, 100);
    doc.depth = 3;
    doc.shapes.push_back(Shape::fromRect("b", QRectF(10, 10, 20, 20), false));
    doc.shapes.push_back(Shape::fromRect("a", QRectF(50, 50, 20, 20), false));

    const QString txtPath = dir.filePath("image.txt");
    QVERIFY(AnnotationIO::saveYolo(txtPath, doc, {}));

    QFile classes(dir.filePath("classes.txt"));
    QVERIFY(classes.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(classes.readAll()), QString("b\na\n"));
}

void CoreTests::createMlUpdatesExistingImageEntry() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AnnotationDocument first;
    first.imagePath = dir.filePath("image.jpg");
    first.imageSize = QSize(100, 100);
    first.shapes.push_back(Shape::fromRect("old", QRectF(0, 0, 10, 10), false));

    const QString jsonPath = dir.filePath("annotations.json");
    QVERIFY(AnnotationIO::saveCreateMl(jsonPath, first));

    AnnotationDocument second = first;
    second.shapes.clear();
    second.shapes.push_back(Shape::fromRect("new", QRectF(20, 20, 30, 30), false));
    QVERIFY(AnnotationIO::saveCreateMl(jsonPath, second));

    AnnotationDocument loaded;
    QVERIFY(AnnotationIO::loadCreateMl(jsonPath, first.imagePath, &loaded));
    QCOMPARE(loaded.shapes.size(), 1);
    QCOMPARE(loaded.shapes[0].label, QString("new"));
}

void CoreTests::labelNavigationSelectsTopWhenEmptyAndWraps() {
    LabelListModel labels;
    labels.add(Shape::fromRect("first", QRectF(0, 0, 10, 10), false));
    labels.add(Shape::fromRect("second", QRectF(20, 20, 10, 10), false));
    labels.clearSelection();

    labels.selectAdjacent(1);
    QCOMPARE(labels.currentIndex(), 0);
    labels.selectAdjacent(1);
    QCOMPARE(labels.currentIndex(), 1);
    labels.selectAdjacent(1);
    QCOMPARE(labels.currentIndex(), 0);
    labels.selectAdjacent(-1);
    QCOMPARE(labels.currentIndex(), 1);

    labels.removeCurrent();
    QCOMPARE(labels.count(), 1);
    QCOMPARE(labels.currentIndex(), 0);
    QCOMPARE(labels.current().label, QString("first"));
}

void CoreTests::shapeSupportsVertexEditingAndCopy() {
    Shape shape = Shape::fromRect("box", QRectF(10, 20, 30, 40), false);

    QCOMPARE(shape.nearestVertex(QPointF(10.5, 20.5), 2.0), 0);
    QCOMPARE(shape.nearestVertex(QPointF(80, 80), 2.0), -1);

    shape.moveVertexBy(0, QPointF(-5, -10));
    QCOMPARE(shape.boundingRect().toRect(), QRect(5, 10, 35, 50));
    QCOMPARE(shape.points[1].y(), 10.0);
    QCOMPARE(shape.points[3].x(), 5.0);

    Shape copy = shape.copy();
    copy.moveBy(QPointF(3, 4));
    QCOMPARE(shape.boundingRect().topLeft(), QPointF(5, 10));
    QCOMPARE(copy.boundingRect().topLeft(), QPointF(8, 14));

    shape.setVisible(false);
    QVERIFY(!shape.visible);
    QVERIFY(!shape.contains(QPointF(15, 15)));
}

void CoreTests::performanceMonitorReturnsCpuAndMemoryText() {
    PerformanceMonitor monitor;
    const QString text = monitor.getPerformanceText();

    QVERIFY(text.contains("CPU"));
    QVERIFY(text.contains("MEM"));
    QVERIFY(!text.isEmpty());
}

QTEST_MAIN(CoreTests)
#include "test_core.moc"
