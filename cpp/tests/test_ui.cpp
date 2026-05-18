#include <QtTest>
#include <QtWidgets>

#include "core/AnnotationIO.h"
#include "ui/Canvas.h"
#include "ui/MiniMapOverlay.h"
#include "ui/MainWindow.h"

class UiTests : public QObject {
    Q_OBJECT

private slots:
    void canvasCreatesAndCancelsShapes();
    void canvasCreateModeUsesCrossCursorAndIgnoresTinyClicks();
    void canvasRightDragPansButRightClickOpensMenu();
    void canvasIgnoresRightDragBelowThreshold();
    void canvasRightDragDoesNotJumpWhenThresholdIsCrossed();
    void canvasWheelZoomReportsAnchorPoint();
    void canvasCopiesAndMovesCurrentShapeToPoint();
    void canvasScaleClampSamplingAndOverview();
    void mainWindowLanguageActionRefreshesTexts();
    void mainWindowUsesFramelessChrome();
    void mainWindowEmbedsToolbarIntoFramelessTitleBar();
    void mainWindowTitleToolbarUsesIconOnlyButtons();
    void mainWindowShowsPerformanceAndMiniMapControls();
    void mainWindowMiniMapClickChangesScrollbars();
    void mainWindowFileListUsesReadableSelectionStyleAndTopDock();
    void mainWindowStartupDirectoryPopulatesFileList();
    void mainWindowStartupDirectoryUsesSupportedFormatsAndNaturalSort();
    void mainWindowNextPreviousKeepsFileListSelection();
    void mainWindowRestoresPersistedLabelHistory();
    void mainWindowRestoresDefaultLabelSettings();
    void mainWindowInlineLabelEditUpdatesLabelHistory();
    void mainWindowLabelFilterOnlyTogglesShapeVisibility();
};

namespace {
void resetTestSettings(const QString &name) {
    QCoreApplication::setOrganizationName("labelImgCppUiTests");
    QCoreApplication::setApplicationName(name);
    QSettings settings;
    settings.clear();
}

QAction *actionByShortcut(QObject *root, const QKeySequence &shortcut) {
    const QList<QAction *> actions = root->findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action->shortcut() == shortcut) {
            return action;
        }
    }
    return nullptr;
}

QAction *languageAction(QObject *root, const QString &language) {
    const QList<QAction *> actions = root->findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action->data().toString() == language) {
            return action;
        }
    }
    return nullptr;
}
}

void UiTests::canvasCreatesAndCancelsShapes() {
    Canvas canvas;
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);
    canvas.setCreateMode(true);
    canvas.resize(120, 120);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
    QTest::mouseMove(&canvas, QPoint(50, 40));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(50, 40));

    QCOMPARE(canvas.shapes().size(), 1);
    QCOMPARE(canvas.currentIndex(), 0);
    QCOMPARE(canvas.shapes().first().boundingRect().toRect(), QRect(10, 10, 40, 30));

    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(60, 60));
    QTest::mouseMove(&canvas, QPoint(80, 80));
    QTest::keyClick(&canvas, Qt::Key_Escape);

    QCOMPARE(canvas.shapes().size(), 1);
}

void UiTests::canvasCreateModeUsesCrossCursorAndIgnoresTinyClicks() {
    Canvas canvas;
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);
    canvas.setCreateMode(true);
    canvas.resize(120, 120);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QCOMPARE(canvas.cursor().shape(), Qt::CrossCursor);

    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(15, 15));
    QCOMPARE(canvas.shapes().size(), 0);

    QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QTest::mouseMove(&canvas, QPoint(22, 22));
    QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(22, 22));
    QCOMPARE(canvas.shapes().size(), 0);
}

void UiTests::canvasRightDragPansButRightClickOpensMenu() {
    Canvas canvas;
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);
    canvas.resize(120, 120);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QSignalSpy scrollSpy(&canvas, &Canvas::scrollRequested);
    QSignalSpy menuSpy(&canvas, &Canvas::contextMenuRequested);

    QTest::mousePress(&canvas, Qt::RightButton, Qt::NoModifier, QPoint(20, 20));
    QTest::mouseMove(&canvas, QPoint(45, 45));
    QTest::mouseMove(&canvas, QPoint(48, 47));
    QTest::mouseRelease(&canvas, Qt::RightButton, Qt::NoModifier, QPoint(48, 47));

    QVERIFY(scrollSpy.count() > 0);
    QCOMPARE(menuSpy.count(), 0);

    QTest::mouseClick(&canvas, Qt::RightButton, Qt::NoModifier, QPoint(30, 30));
    QCOMPARE(menuSpy.count(), 1);
}

void UiTests::canvasIgnoresRightDragBelowThreshold() {
    Canvas canvas;
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);
    canvas.resize(120, 120);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QSignalSpy scrollSpy(&canvas, &Canvas::scrollRequested);
    QSignalSpy menuSpy(&canvas, &Canvas::contextMenuRequested);

    QTest::mousePress(&canvas, Qt::RightButton, Qt::NoModifier, QPoint(20, 20));
    QTest::mouseMove(&canvas, QPoint(22, 22));
    QTest::mouseRelease(&canvas, Qt::RightButton, Qt::NoModifier, QPoint(22, 22));

    QCOMPARE(scrollSpy.count(), 0);
    QCOMPARE(menuSpy.count(), 1);
}

void UiTests::canvasRightDragDoesNotJumpWhenThresholdIsCrossed() {
    Canvas canvas;
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);
    canvas.resize(120, 120);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QSignalSpy scrollSpy(&canvas, &Canvas::scrollRequested);

    QTest::mousePress(&canvas, Qt::RightButton, Qt::NoModifier, QPoint(20, 20));
    QTest::mouseMove(&canvas, QPoint(23, 23));
    QTest::mouseMove(&canvas, QPoint(36, 36));
    QTest::mouseMove(&canvas, QPoint(38, 37));
    QTest::mouseRelease(&canvas, Qt::RightButton, Qt::NoModifier, QPoint(38, 37));

    QCOMPARE(scrollSpy.count(), 1);
    QCOMPARE(scrollSpy.first().at(0).toInt(), 2);
    QCOMPARE(scrollSpy.first().at(1).toInt(), 1);
}

void UiTests::canvasWheelZoomReportsAnchorPoint() {
    Canvas canvas;
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);
    canvas.resize(120, 120);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    QSignalSpy scaleSpy(&canvas, &Canvas::scaleChanged);
    QWheelEvent event(QPointF(40, 50), canvas.mapToGlobal(QPoint(40, 50)),
                      QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
    QApplication::sendEvent(&canvas, &event);

    QCOMPARE(scaleSpy.count(), 1);
    QCOMPARE(scaleSpy.first().at(0).toDouble(), 1.0);
    QCOMPARE(scaleSpy.first().at(1).toDouble(), 1.15);
    QCOMPARE(scaleSpy.first().at(2).toPoint(), QPoint(40, 50));
}

void UiTests::canvasCopiesAndMovesCurrentShapeToPoint() {
    Canvas canvas;
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);
    canvas.setShapes({Shape::fromRect("box", QRectF(10, 10, 20, 20), false)});
    canvas.setCurrentIndex(0);

    QVERIFY(canvas.copyCurrentTo(QPointF(80, 80)));
    QCOMPARE(canvas.shapes().size(), 2);
    QCOMPARE(canvas.currentIndex(), 1);
    QCOMPARE(canvas.shapes().last().boundingRect().center(), QPointF(80, 80));

    QVERIFY(canvas.moveCurrentTo(QPointF(10, 10)));
    QCOMPARE(canvas.shapes().size(), 2);
    QCOMPARE(canvas.shapes().last().boundingRect().center(), QPointF(10, 10));
}

void UiTests::canvasScaleClampSamplingAndOverview() {
    Canvas canvas;
    QPixmap pixmap(400, 200);
    pixmap.fill(Qt::white);
    canvas.setPixmap(pixmap);

    canvas.setScale(0.001);
    QCOMPARE(canvas.scale(), 0.005);
    canvas.setScale(20.0);
    QCOMPARE(canvas.scale(), 16.0);

    QCOMPARE(canvas.samplingMode(), Canvas::SamplingMode::FastNearest);
    canvas.setSamplingMode(Canvas::SamplingMode::Smooth);
    QCOMPARE(canvas.samplingMode(), Canvas::SamplingMode::Smooth);

    QImage overview = canvas.overviewImage(128);
    QVERIFY(!overview.isNull());
    QCOMPARE(qMax(overview.width(), overview.height()), 128);
    QCOMPARE(overview.size(), QSize(128, 64));

    QSignalSpy frameSpy(&canvas, &Canvas::frameRendered);
    canvas.resize(160, 120);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));
    canvas.update();
    QTest::qWait(50);
    QVERIFY(frameSpy.count() > 0);
}

void UiTests::mainWindowLanguageActionRefreshesTexts() {
    resetTestSettings("language-switch");
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QAction *openAction = actionByShortcut(&window, QKeySequence::Open);
    QVERIFY(openAction);
    QAction *zhAction = languageAction(&window, "zh-CN");
    QVERIFY(zhAction);

    zhAction->trigger();
    QCOMPARE(openAction->text(), QString::fromUtf8("打开文件"));
}

void UiTests::mainWindowUsesFramelessChrome() {
    resetTestSettings("frameless-chrome");
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.windowFlags().testFlag(Qt::FramelessWindowHint));
    QWidget *titleBar = window.findChild<QWidget *>("framelessTitleBar");
    QVERIFY(titleBar);
    QVERIFY(window.findChild<QToolButton *>("minimizeButton"));
    QVERIFY(window.findChild<QToolButton *>("maximizeButton"));
    QVERIFY(window.findChild<QToolButton *>("closeButton"));
    QVERIFY(window.menuWidget());
}

void UiTests::mainWindowEmbedsToolbarIntoFramelessTitleBar() {
    resetTestSettings("frameless-toolbar");
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QWidget *titleBar = window.findChild<QWidget *>("framelessTitleBar");
    QToolBar *toolBar = window.findChild<QToolBar *>("mainToolBar");
    QVERIFY(titleBar);
    QVERIFY(toolBar);
    QVERIFY(titleBar->isAncestorOf(toolBar));
    QCOMPARE(window.toolBarArea(toolBar), Qt::NoToolBarArea);
    QVERIFY(toolBar->findChildren<QToolButton *>().size() > 3);
}

void UiTests::mainWindowTitleToolbarUsesIconOnlyButtons() {
    resetTestSettings("title-toolbar-icons");
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QToolBar *toolBar = window.findChild<QToolBar *>("mainToolBar");
    QVERIFY(toolBar);
    QCOMPARE(toolBar->toolButtonStyle(), Qt::ToolButtonIconOnly);
    QCOMPARE(toolBar->iconSize(), QSize(20, 20));

    int actionCount = 0;
    for (QAction *action : toolBar->actions()) {
        if (action->isSeparator()) {
            continue;
        }
        ++actionCount;
        QVERIFY2(!action->icon().isNull(), qPrintable(action->text()));
        QVERIFY2(!action->toolTip().isEmpty(), qPrintable(action->text()));
    }
    QVERIFY(actionCount > 3);
}

void UiTests::mainWindowShowsPerformanceAndMiniMapControls() {
    resetTestSettings("perf-minimap-controls");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QImage image(120, 80, QImage::Format_RGB32);
    image.fill(Qt::white);
    const QString imagePath = dir.filePath("image.jpg");
    QVERIFY(image.save(imagePath));

    MainWindow window;
    window.loadStartupArgs({"labelImgCpp", imagePath});
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QLabel *performanceLabel = window.findChild<QLabel *>("performanceLabel");
    QAction *showPerformanceAction = window.findChild<QAction *>("showPerformanceAction");
    QAction *miniMapAction = window.findChild<QAction *>("miniMapAction");
    QAction *samplingModeAction = window.findChild<QAction *>("samplingModeAction");
    MiniMapOverlay *miniMap = window.findChild<MiniMapOverlay *>("miniMapOverlay");
    QVERIFY(performanceLabel);
    QVERIFY(showPerformanceAction);
    QVERIFY(miniMapAction);
    QVERIFY(samplingModeAction);
    QVERIFY(miniMap);

    QVERIFY(performanceLabel->isVisibleTo(&window));
    QVERIFY(miniMap->isVisible());
    showPerformanceAction->trigger();
    QVERIFY(!performanceLabel->isVisible());
    miniMapAction->trigger();
    QVERIFY(!miniMap->isVisible());

    QSettings settings;
    QCOMPARE(settings.value("view/showPerformance").toBool(), false);
    QCOMPARE(settings.value("view/miniMapEnabled").toBool(), false);
}

void UiTests::mainWindowMiniMapClickChangesScrollbars() {
    resetTestSettings("minimap-scroll");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QImage image(800, 600, QImage::Format_RGB32);
    image.fill(Qt::white);
    const QString imagePath = dir.filePath("large.jpg");
    QVERIFY(image.save(imagePath));

    MainWindow window;
    window.resize(500, 400);
    window.loadStartupArgs({"labelImgCpp", imagePath});
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    Canvas *canvas = window.findChild<Canvas *>();
    QScrollArea *scrollArea = window.findChild<QScrollArea *>();
    MiniMapOverlay *miniMap = window.findChild<MiniMapOverlay *>("miniMapOverlay");
    QVERIFY(canvas);
    QVERIFY(scrollArea);
    QVERIFY(miniMap);

    canvas->setScale(2.0);
    QApplication::processEvents();
    QVERIFY(scrollArea->horizontalScrollBar()->maximum() > 0);
    QVERIFY(scrollArea->verticalScrollBar()->maximum() > 0);

    const int oldH = scrollArea->horizontalScrollBar()->value();
    const int oldV = scrollArea->verticalScrollBar()->value();
    QPoint clickPoint = miniMap->rect().center();
    QTest::mouseClick(miniMap, Qt::LeftButton, Qt::NoModifier, clickPoint);

    QVERIFY(scrollArea->horizontalScrollBar()->value() != oldH ||
            scrollArea->verticalScrollBar()->value() != oldV);
}

void UiTests::mainWindowFileListUsesReadableSelectionStyleAndTopDock() {
    resetTestSettings("file-list-style");
    MainWindow window;
    QListWidget *fileList = window.findChild<QListWidget *>("fileList");
    QDockWidget *fileDock = window.findChild<QDockWidget *>("files");
    QDockWidget *labelDock = window.findChild<QDockWidget *>("labels");
    QVERIFY(fileList);
    QVERIFY(fileDock);
    QVERIFY(labelDock);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(fileList->font().pointSize() >= 10);
    QVERIFY(fileList->styleSheet().contains("QListWidget::item:selected"));
    QVERIFY(fileList->styleSheet().contains("background"));
    QCOMPARE(window.dockWidgetArea(fileDock), Qt::RightDockWidgetArea);
    QCOMPARE(window.dockWidgetArea(labelDock), Qt::RightDockWidgetArea);
    QVERIFY(fileDock->geometry().top() <= labelDock->geometry().top());
}

void UiTests::mainWindowStartupDirectoryPopulatesFileList() {
    resetTestSettings("startup-directory");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QImage first(10, 10, QImage::Format_RGB32);
    first.fill(Qt::white);
    QImage second(10, 10, QImage::Format_RGB32);
    second.fill(Qt::black);
    QVERIFY(first.save(dir.filePath("b.jpg")));
    QVERIFY(second.save(dir.filePath("a.jpg")));

    MainWindow window;
    window.loadStartupArgs({"labelImgCpp", dir.path()});
    QListWidget *fileList = window.findChild<QListWidget *>("fileList");
    QVERIFY(fileList);

    QCOMPARE(fileList->count(), 2);
    QVERIFY(fileList->currentItem());
    QCOMPARE(QFileInfo(fileList->currentItem()->text()).fileName(), QString("a.jpg"));
}

void UiTests::mainWindowStartupDirectoryUsesSupportedFormatsAndNaturalSort() {
    resetTestSettings("startup-directory-natural-sort");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QImage image(10, 10, QImage::Format_RGB32);
    image.fill(Qt::white);
    QVERIFY(image.save(dir.filePath("img10.jpg")));
    QVERIFY(image.save(dir.filePath("img2.jpg")));
    QVERIFY(image.save(dir.filePath("img1.ppm")));

    MainWindow window;
    window.loadStartupArgs({"labelImgCpp", dir.path()});
    QListWidget *fileList = window.findChild<QListWidget *>("fileList");
    QVERIFY(fileList);

    QCOMPARE(fileList->count(), 3);
    QCOMPARE(QFileInfo(fileList->item(0)->text()).fileName(), QString("img1.ppm"));
    QCOMPARE(QFileInfo(fileList->item(1)->text()).fileName(), QString("img2.jpg"));
    QCOMPARE(QFileInfo(fileList->item(2)->text()).fileName(), QString("img10.jpg"));
}

void UiTests::mainWindowNextPreviousKeepsFileListSelection() {
    resetTestSettings("next-previous-selection");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QImage first(10, 10, QImage::Format_RGB32);
    first.fill(Qt::white);
    QImage second(10, 10, QImage::Format_RGB32);
    second.fill(Qt::black);
    QVERIFY(first.save(dir.filePath("a.jpg")));
    QVERIFY(second.save(dir.filePath("b.jpg")));

    MainWindow window;
    window.loadStartupArgs({"labelImgCpp", dir.path()});
    QListWidget *fileList = window.findChild<QListWidget *>("fileList");
    QVERIFY(fileList);
    QAction *nextAction = actionByShortcut(&window, QKeySequence("D"));
    QAction *prevAction = actionByShortcut(&window, QKeySequence("A"));
    QVERIFY(nextAction);
    QVERIFY(prevAction);

    nextAction->trigger();
    QVERIFY(fileList->currentItem());
    QCOMPARE(QFileInfo(fileList->currentItem()->text()).fileName(), QString("b.jpg"));

    prevAction->trigger();
    QVERIFY(fileList->currentItem());
    QCOMPARE(QFileInfo(fileList->currentItem()->text()).fileName(), QString("a.jpg"));
}

void UiTests::mainWindowRestoresPersistedLabelHistory() {
    resetTestSettings("label-history");
    QSettings settings;
    settings.setValue("labelHistory", QStringList{QString::fromUtf8("自定义缺陷")});

    MainWindow window;
    QComboBox *defaultLabelCombo = window.findChild<QComboBox *>("defaultLabelCombo");
    QVERIFY(defaultLabelCombo);

    QVERIFY(defaultLabelCombo->findText(QString::fromUtf8("自定义缺陷")) >= 0);
}

void UiTests::mainWindowRestoresDefaultLabelSettings() {
    resetTestSettings("default-label");
    QSettings settings;
    settings.setValue("labelHistory", QStringList{QString::fromUtf8("缺陷A")});
    settings.setValue("useDefaultLabel", true);
    settings.setValue("defaultLabel", QString::fromUtf8("缺陷A"));

    MainWindow window;
    QCheckBox *useDefaultLabel = window.findChild<QCheckBox *>("useDefaultLabel");
    QComboBox *defaultLabelCombo = window.findChild<QComboBox *>("defaultLabelCombo");
    QVERIFY(useDefaultLabel);
    QVERIFY(defaultLabelCombo);

    QVERIFY(useDefaultLabel->isChecked());
    QCOMPARE(defaultLabelCombo->currentText(), QString::fromUtf8("缺陷A"));
}

void UiTests::mainWindowInlineLabelEditUpdatesLabelHistory() {
    resetTestSettings("inline-label-history");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QImage image(40, 40, QImage::Format_RGB32);
    image.fill(Qt::white);
    const QString imagePath = dir.filePath("a.jpg");
    QVERIFY(image.save(imagePath));

    AnnotationDocument doc;
    doc.imagePath = imagePath;
    doc.imageSize = image.size();
    doc.depth = 3;
    doc.shapes = {Shape::fromRect("old", QRectF(5, 5, 10, 10), false)};
    QVERIFY(AnnotationIO::savePascalVoc(dir.filePath("a.xml"), doc));

    MainWindow window;
    window.loadStartupArgs({"labelImgCpp", dir.path()});
    QListWidget *labelList = window.findChild<QListWidget *>("labelList");
    QComboBox *defaultLabelCombo = window.findChild<QComboBox *>("defaultLabelCombo");
    QVERIFY(labelList);
    QVERIFY(defaultLabelCombo);
    QCOMPARE(labelList->count(), 1);

    labelList->item(0)->setText(QString::fromUtf8("新标签"));

    QVERIFY(defaultLabelCombo->findText(QString::fromUtf8("新标签")) >= 0);
    QSettings settings;
    QVERIFY(settings.value("labelHistory").toStringList().contains(QString::fromUtf8("新标签")));
}

void UiTests::mainWindowLabelFilterOnlyTogglesShapeVisibility() {
    resetTestSettings("label-filter");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QImage image(40, 40, QImage::Format_RGB32);
    image.fill(Qt::white);
    const QString imagePath = dir.filePath("a.jpg");
    QVERIFY(image.save(imagePath));

    AnnotationDocument doc;
    doc.imagePath = imagePath;
    doc.imageSize = image.size();
    doc.depth = 3;
    doc.shapes = {Shape::fromRect("keep", QRectF(5, 5, 10, 10), false),
                  Shape::fromRect("hide", QRectF(20, 20, 10, 10), false)};
    QVERIFY(AnnotationIO::savePascalVoc(dir.filePath("a.xml"), doc));

    MainWindow window;
    window.loadStartupArgs({"labelImgCpp", dir.path()});
    QComboBox *filterCombo = window.findChild<QComboBox *>("filterCombo");
    QListWidget *labelList = window.findChild<QListWidget *>("labelList");
    Canvas *canvas = window.findChild<Canvas *>();
    QVERIFY(filterCombo);
    QVERIFY(labelList);
    QVERIFY(canvas);
    QCOMPARE(labelList->count(), 2);

    filterCombo->setCurrentText("keep");

    QCOMPARE(labelList->item(0)->checkState(), Qt::Checked);
    QCOMPARE(labelList->item(1)->checkState(), Qt::Unchecked);
    QVERIFY(canvas->shapes().at(0).visible);
    QVERIFY(!canvas->shapes().at(1).visible);
    QCOMPARE(filterCombo->currentText(), QString("keep"));
}

QTEST_MAIN(UiTests)
#include "test_ui.moc"
