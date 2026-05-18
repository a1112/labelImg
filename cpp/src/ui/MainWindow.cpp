#include "ui/MainWindow.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QColorDialog>
#include <QCoreApplication>
#include <QDockWidget>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QIcon>
#include <QImageReader>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#endif

namespace {
QString repoRoot() {
#ifdef LABELIMG_REPO_ROOT
    return QString::fromUtf8(LABELIMG_REPO_ROOT);
#else
    return QDir::currentPath();
#endif
}

QIcon resourceIcon(const QString &fileName) {
    return QIcon(QDir(repoRoot()).filePath(QStringLiteral("resources/icons/") + fileName));
}

QColor colorForLabel(const QString &label) {
    uint hash = qHash(label);
    return QColor::fromHsv(hash % 359, 160, 230);
}

enum class HitRegion {
    Client,
    Caption,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
};

HitRegion hitRegionFor(const QRect &windowFrame, const QRect &titleFrame, const QPoint &globalPos, int border) {
    const bool left = globalPos.x() >= windowFrame.left() && globalPos.x() < windowFrame.left() + border;
    const bool right = globalPos.x() <= windowFrame.right() && globalPos.x() > windowFrame.right() - border;
    const bool top = globalPos.y() >= windowFrame.top() && globalPos.y() < windowFrame.top() + border;
    const bool bottom = globalPos.y() <= windowFrame.bottom() && globalPos.y() > windowFrame.bottom() - border;

    if (top && left) return HitRegion::TopLeft;
    if (top && right) return HitRegion::TopRight;
    if (bottom && left) return HitRegion::BottomLeft;
    if (bottom && right) return HitRegion::BottomRight;
    if (left) return HitRegion::Left;
    if (right) return HitRegion::Right;
    if (top) return HitRegion::Top;
    if (bottom) return HitRegion::Bottom;
    if (titleFrame.contains(globalPos)) return HitRegion::Caption;
    return HitRegion::Client;
}

bool naturalPathLess(const QString &left, const QString &right) {
    const QString a = left.toLower();
    const QString b = right.toLower();
    int i = 0;
    int j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i].isDigit() && b[j].isDigit()) {
            int aStart = i;
            int bStart = j;
            while (i < a.size() && a[i].isDigit()) ++i;
            while (j < b.size() && b[j].isDigit()) ++j;

            QString aNumber = a.mid(aStart, i - aStart);
            QString bNumber = b.mid(bStart, j - bStart);
            while (aNumber.size() > 1 && aNumber.startsWith('0')) aNumber.remove(0, 1);
            while (bNumber.size() > 1 && bNumber.startsWith('0')) bNumber.remove(0, 1);

            if (aNumber.size() != bNumber.size()) return aNumber.size() < bNumber.size();
            int numericCompare = QString::compare(aNumber, bNumber);
            if (numericCompare != 0) return numericCompare < 0;

            int originalLengthCompare = (i - aStart) - (j - bStart);
            if (originalLengthCompare != 0) return originalLengthCompare < 0;
            continue;
        }

        if (a[i] != b[j]) return a[i] < b[j];
        ++i;
        ++j;
    }
    return a.size() < b.size();
}

QStringList supportedImageNameFilters() {
    QStringList filters;
    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    for (const QByteArray &format : formats) {
        QString suffix = QString::fromLatin1(format).toLower();
        if (!suffix.isEmpty()) {
            QString filter = "*." + suffix;
            if (!filters.contains(filter)) filters.append(filter);
        }
    }
    return filters;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);
    loadSettings();
    loadPredefinedClasses();
    createUi();
    createActions();
    createMenusAndToolbars();
    installFramelessChrome();
    connectSignals();
    setAdvancedMode(m_advancedModeAction->isChecked());
    refreshTexts();
    refreshActions();
    resize(m_settings.value("window/size", QSize(1100, 700)).toSize());
    move(m_settings.value("window/position", QPoint(20, 20)).toPoint());
    restoreState(m_settings.value("window/state").toByteArray());
    loadStartupArgs(QCoreApplication::arguments());
}

void MainWindow::loadStartupArgs(const QStringList &arguments) {
    if (arguments.size() > 2) {
        const QString savedDefaultLabel = m_settings.value("defaultLabel").toString();
        loadPredefinedClassesFromFile(arguments[2]);
        mergePersistedLabelHistory();
        QSignalBlocker blocker(m_defaultLabelCombo);
        m_defaultLabelCombo->clear();
        m_defaultLabelCombo->addItems(m_classList);
        int defaultLabelIndex = m_defaultLabelCombo->findText(savedDefaultLabel);
        if (defaultLabelIndex >= 0) {
            m_defaultLabelCombo->setCurrentIndex(defaultLabelIndex);
        }
    }
    if (arguments.size() > 3) {
        m_saveDir = QFileInfo(arguments[3]).absoluteFilePath();
    }
    if (arguments.size() > 1 && QFileInfo::exists(arguments[1])) {
        QFileInfo startupInfo(arguments[1]);
        if (startupInfo.isDir()) {
            m_dirPath = startupInfo.absoluteFilePath();
            m_settings.setValue("lastOpenDir", m_dirPath);
            m_imageList = scanImages(m_dirPath);
            m_fileList->clear();
            for (const QString &path : m_imageList) m_fileList->addItem(path);
            m_currentImageIndex = 0;
            if (!m_imageList.isEmpty()) loadImage(m_imageList.first());
        } else {
            loadImage(arguments[1]);
        }
    } else {
        QString lastFile = m_settings.value("filename").toString();
        if (!lastFile.isEmpty() && QFileInfo::exists(lastFile)) {
            loadImage(lastFile);
        }
    }
}

void MainWindow::createUi() {
    m_canvas = new Canvas(this);
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_canvas);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    setCentralWidget(m_scrollArea);

    m_labelList = new QListWidget(this);
    m_labelList->setObjectName("labelList");
    m_filterCombo = new QComboBox(this);
    m_filterCombo->setObjectName("filterCombo");
    m_defaultLabelCombo = new QComboBox(this);
    m_defaultLabelCombo->setObjectName("defaultLabelCombo");
    m_defaultLabelCombo->addItems(m_classList);
    m_useDefaultLabel = new QCheckBox(this);
    m_useDefaultLabel->setObjectName("useDefaultLabel");
    m_useDefaultLabel->setChecked(m_settings.value("useDefaultLabel", false).toBool());
    m_difficult = new QCheckBox(this);
    const QString defaultLabel = m_settings.value("defaultLabel").toString();
    int defaultLabelIndex = m_defaultLabelCombo->findText(defaultLabel);
    if (defaultLabelIndex >= 0) {
        m_defaultLabelCombo->setCurrentIndex(defaultLabelIndex);
    }

    QWidget *labelPanel = new QWidget(this);
    QVBoxLayout *labelLayout = new QVBoxLayout(labelPanel);
    labelLayout->setContentsMargins(0, 0, 0, 0);
    labelLayout->addWidget(m_useDefaultLabel);
    labelLayout->addWidget(m_defaultLabelCombo);
    labelLayout->addWidget(m_difficult);
    labelLayout->addWidget(m_filterCombo);
    labelLayout->addWidget(m_labelList);
    m_fileList = new QListWidget(this);
    m_fileList->setObjectName("fileList");
    QFont fileFont = m_fileList->font();
    fileFont.setPointSize(qMax(fileFont.pointSize(), 10));
    m_fileList->setFont(fileFont);
    m_fileList->setStyleSheet(QStringLiteral(
        "QListWidget { outline: 0; }"
        "QListWidget::item { padding: 5px 8px; min-height: 24px; }"
        "QListWidget::item:selected { background: #d8ecff; color: #111827; border-left: 3px solid #2f80ed; }"
        "QListWidget::item:selected:active { background: #c7e3ff; }"
        "QListWidget::item:hover { background: #edf6ff; }"));
    m_fileDock = new QDockWidget(this);
    m_fileDock->setObjectName("files");
    m_fileDock->setWidget(m_fileList);
    addDockWidget(Qt::RightDockWidgetArea, m_fileDock);

    m_labelDock = new QDockWidget(this);
    m_labelDock->setObjectName("labels");
    m_labelDock->setWidget(labelPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_labelDock);
    splitDockWidget(m_fileDock, m_labelDock, Qt::Vertical);

    m_coordinates = new QLabel(this);
    m_performanceLabel = new QLabel(this);
    m_performanceLabel->setObjectName("performanceLabel");
    m_performanceLabel->setStyleSheet(QStringLiteral("QLabel { color: #777; padding: 0 8px; }"));
    statusBar()->addWidget(m_performanceLabel, 0);
    statusBar()->addPermanentWidget(m_coordinates);

    m_miniMapOverlay = new MiniMapOverlay(m_canvas, m_scrollArea, m_scrollArea->viewport());
    m_miniMapOverlay->setObjectName("miniMapOverlay");
    m_performanceMonitor = new PerformanceMonitor(this);
    updatePerformanceLabel();
}

void MainWindow::createActions() {
    m_openAction = new QAction(this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openDirAction = new QAction(this);
    m_openDirAction->setShortcut(QKeySequence("Ctrl+U"));
    m_openAnnotationAction = new QAction(this);
    m_openAnnotationAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    m_closeAction = new QAction(this);
    m_closeAction->setShortcut(QKeySequence("Ctrl+W"));
    m_resetAllAction = new QAction(this);
    m_saveAction = new QAction(this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAsAction = new QAction(this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_changeSaveDirAction = new QAction(this);
    m_changeSaveDirAction->setShortcut(QKeySequence("Ctrl+R"));
    m_formatAction = new QAction(this);
    m_formatAction->setShortcut(QKeySequence("Ctrl+Y"));
    m_nextAction = new QAction(this);
    m_nextAction->setShortcut(QKeySequence("D"));
    m_prevAction = new QAction(this);
    m_prevAction->setShortcut(QKeySequence("A"));
    m_verifyAction = new QAction(this);
    m_verifyAction->setShortcut(QKeySequence("Space"));
    m_verifyAction->setCheckable(true);
    m_editLabelAction = new QAction(this);
    m_editLabelAction->setShortcut(QKeySequence("Ctrl+E"));
    m_deleteAction = new QAction(this);
    m_deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
    m_copyAction = new QAction(this);
    m_copyAction->setShortcut(QKeySequence("Ctrl+D"));
    m_copyHereAction = new QAction("&Copy here", this);
    m_moveHereAction = new QAction("&Move here", this);
    m_copyPreviousAction = new QAction(this);
    m_copyPreviousAction->setShortcut(QKeySequence("Ctrl+V"));
    m_deleteImageAction = new QAction(this);
    m_deleteImageAction->setShortcut(QKeySequence("Ctrl+Delete"));
    m_createModeAction = new QAction(this);
    m_createModeAction->setShortcut(QKeySequence("W"));
    m_createModeAction->setCheckable(true);
    m_createModeAction->setChecked(false);
    m_editModeAction = new QAction(this);
    m_editModeAction->setShortcut(QKeySequence("Ctrl+J"));
    m_editModeAction->setCheckable(true);
    m_editModeAction->setChecked(true);
    m_advancedModeAction = new QAction(this);
    m_advancedModeAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    m_advancedModeAction->setCheckable(true);
    m_advancedModeAction->setChecked(m_settings.value("advanced", false).toBool());
    m_hideAllAction = new QAction(this);
    m_hideAllAction->setShortcut(QKeySequence("Ctrl+H"));
    m_showAllAction = new QAction(this);
    m_showAllAction->setShortcut(QKeySequence("Ctrl+A"));
    m_zoomInAction = new QAction(this);
    m_zoomInAction->setShortcut(QKeySequence("Ctrl++"));
    m_zoomOutAction = new QAction(this);
    m_zoomOutAction->setShortcut(QKeySequence("Ctrl+-"));
    m_zoomOriginalAction = new QAction(this);
    m_zoomOriginalAction->setShortcut(QKeySequence("Ctrl+="));
    m_fitWindowAction = new QAction(this);
    m_fitWindowAction->setShortcut(QKeySequence("Ctrl+F"));
    m_fitWindowAction->setCheckable(true);
    m_fitWidthAction = new QAction(this);
    m_fitWidthAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    m_fitWidthAction->setCheckable(true);
    m_brightenAction = new QAction(this);
    m_brightenAction->setShortcut(QKeySequence("Ctrl+Shift++"));
    m_darkenAction = new QAction(this);
    m_darkenAction->setShortcut(QKeySequence("Ctrl+Shift+-"));
    m_brightnessOriginalAction = new QAction(this);
    m_brightnessOriginalAction->setShortcut(QKeySequence("Ctrl+Shift+="));
    m_drawSquareAction = new QAction(this);
    m_drawSquareAction->setCheckable(true);
    m_drawSquareAction->setChecked(m_settings.value("draw/square", false).toBool());
    m_boxLineColorAction = new QAction(this);
    m_boxLineColorAction->setShortcut(QKeySequence("Ctrl+L"));
    m_shapeLineColorAction = new QAction(this);
    m_shapeFillColorAction = new QAction(this);
    m_infoAction = new QAction(this);
    m_shortcutsAction = new QAction(this);
    m_autoSaveAction = new QAction(this);
    m_autoSaveAction->setCheckable(true);
    m_autoSaveAction->setChecked(m_settings.value("autosave", false).toBool());
    m_singleClassAction = new QAction(this);
    m_singleClassAction->setCheckable(true);
    m_singleClassAction->setChecked(m_settings.value("singleclass", false).toBool());
    m_displayLabelsAction = new QAction(this);
    m_displayLabelsAction->setCheckable(true);
    m_displayLabelsAction->setChecked(m_settings.value("paintlabel", false).toBool());
    m_miniMapAction = new QAction(this);
    m_miniMapAction->setObjectName("miniMapAction");
    m_miniMapAction->setCheckable(true);
    m_miniMapAction->setChecked(m_settings.value("view/miniMapEnabled", true).toBool());
    m_showPerformanceAction = new QAction(this);
    m_showPerformanceAction->setObjectName("showPerformanceAction");
    m_showPerformanceAction->setCheckable(true);
    m_showPerformanceAction->setChecked(m_settings.value("view/showPerformance", true).toBool());
    m_samplingModeAction = new QAction(this);
    m_samplingModeAction->setObjectName("samplingModeAction");
    m_samplingModeAction->setCheckable(true);
    m_samplingModeAction->setChecked(m_settings.value("view/samplingMode").toString() == "Smooth");
    m_lineColor = m_settings.value("line/color", m_lineColor).value<QColor>();
    m_fillColor = m_settings.value("fill/color", m_fillColor).value<QColor>();
    m_canvas->setLineColor(m_lineColor);
    m_canvas->setFillColor(m_fillColor);
    m_canvas->setDrawSquare(m_drawSquareAction->isChecked());
    m_canvas->setSamplingMode(m_samplingModeAction->isChecked() ? Canvas::SamplingMode::Smooth : Canvas::SamplingMode::FastNearest);
    assignActionIcons();
}

void MainWindow::createMenusAndToolbars() {
    m_fileMenu = menuBar()->addMenu(QString());
    m_fileMenu->addActions({m_openAction, m_openDirAction, m_changeSaveDirAction, m_openAnnotationAction});
    m_recentFilesMenu = m_fileMenu->addMenu(QString());
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_copyPreviousAction);
    m_fileMenu->addActions({m_saveAction, m_formatAction, m_saveAsAction, m_closeAction, m_resetAllAction});
    m_fileMenu->addAction(m_verifyAction);
    m_fileMenu->addAction(m_deleteImageAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_prevAction);
    m_fileMenu->addAction(m_nextAction);

    m_viewMenu = menuBar()->addMenu(QString());
    m_viewMenu->addAction(m_advancedModeAction);
    m_viewMenu->addAction(m_createModeAction);
    m_viewMenu->addAction(m_editModeAction);
    m_viewMenu->addAction(m_autoSaveAction);
    m_viewMenu->addAction(m_singleClassAction);
    m_viewMenu->addAction(m_displayLabelsAction);
    m_viewMenu->addAction(m_drawSquareAction);
    m_viewMenu->addAction(m_boxLineColorAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addActions({m_hideAllAction, m_showAllAction});
    m_viewMenu->addSeparator();
    m_viewMenu->addActions({m_zoomInAction, m_zoomOutAction, m_zoomOriginalAction});
    m_viewMenu->addActions({m_fitWindowAction, m_fitWidthAction});
    m_viewMenu->addActions({m_miniMapAction, m_showPerformanceAction, m_samplingModeAction});
    m_viewMenu->addSeparator();
    m_viewMenu->addActions({m_darkenAction, m_brightenAction, m_brightnessOriginalAction});

    m_languageMenu = m_viewMenu->addMenu(QString());
    QHash<QString, QString> languageNames{{"en", "English"}, {"zh-CN", QString::fromUtf8("简体中文")},
                                          {"zh-TW", QString::fromUtf8("繁體中文")}, {"ja-JP", QString::fromUtf8("日本語")}};
    for (const QString &language : StringBundle::supportedLanguages()) {
        QAction *action = m_languageMenu->addAction(languageNames.value(language, language));
        action->setCheckable(true);
        action->setData(language);
        action->setChecked(language == m_strings.language());
        connect(action, &QAction::triggered, this, [this, language](bool checked) {
            if (checked) changeLanguage(language);
        });
    }

    m_helpMenu = menuBar()->addMenu(QString());
    m_helpMenu->addActions({m_infoAction, m_shortcutsAction});
    m_toolBar = new QToolBar(QStringLiteral("Tools"), this);
    m_toolBar->setObjectName(QStringLiteral("mainToolBar"));
    m_toolBar->setMovable(false);
    m_toolBar->setFloatable(false);
    m_toolBar->setIconSize(QSize(20, 20));
    m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolBar->setStyleSheet(QStringLiteral(
        "QToolBar { background: transparent; border: 0; spacing: 2px; }"
        "QToolButton { min-width: 34px; max-width: 34px; min-height: 30px; padding: 0; border: 1px solid #dde5dc; border-radius: 4px; background: #f8fbf5; }"
        "QToolButton:hover { background: #e9f3e3; }"
        "QToolButton:checked { background: #d7ead0; border-color: #9ac48c; }"));
    populateToolbarForMode();
    rebuildRecentFilesMenu();
}

void MainWindow::installFramelessChrome() {
    if (m_topChrome) {
        return;
    }

    QMenuBar *existingMenuBar = menuBar();
    m_topChrome = new QWidget(this);
    m_topChrome->setObjectName(QStringLiteral("framelessTopChrome"));
    auto *layout = new QVBoxLayout(m_topChrome);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_titleBar = new FramelessTitleBar(m_topChrome);
    m_titleBar->setToolWidget(m_toolBar);
    layout->addWidget(m_titleBar);
    layout->addWidget(existingMenuBar);
    setMenuWidget(m_topChrome);

    connect(m_titleBar, &FramelessTitleBar::dragRequested, this, &MainWindow::startSystemMove);
    connect(m_titleBar, &FramelessTitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_titleBar, &FramelessTitleBar::maximizeRestoreRequested, this, &MainWindow::toggleMaximizeRestore);
    connect(m_titleBar, &FramelessTitleBar::closeRequested, this, &QWidget::close);

    updateFramelessChrome();
}

void MainWindow::connectSignals() {
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(m_openDirAction, &QAction::triggered, this, &MainWindow::openDir);
    connect(m_openAnnotationAction, &QAction::triggered, this, &MainWindow::openAnnotationDialog);
    connect(m_closeAction, &QAction::triggered, this, &MainWindow::closeFile);
    connect(m_resetAllAction, &QAction::triggered, this, &MainWindow::resetAllSettings);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(m_changeSaveDirAction, &QAction::triggered, this, &MainWindow::changeSaveDir);
    connect(m_formatAction, &QAction::triggered, this, &MainWindow::changeFormat);
    connect(m_nextAction, &QAction::triggered, this, &MainWindow::openNextImage);
    connect(m_prevAction, &QAction::triggered, this, &MainWindow::openPrevImage);
    connect(m_verifyAction, &QAction::triggered, this, &MainWindow::verifyImage);
    connect(m_editLabelAction, &QAction::triggered, this, &MainWindow::editCurrentLabel);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteCurrentShape);
    connect(m_copyAction, &QAction::triggered, this, &MainWindow::copyCurrentShape);
    connect(m_copyHereAction, &QAction::triggered, this, &MainWindow::copyShapeHere);
    connect(m_moveHereAction, &QAction::triggered, this, &MainWindow::moveShapeHere);
    connect(m_copyPreviousAction, &QAction::triggered, this, &MainWindow::copyPreviousBoundingBoxes);
    connect(m_deleteImageAction, &QAction::triggered, this, &MainWindow::deleteCurrentImage);
    connect(m_createModeAction, &QAction::triggered, this, &MainWindow::setCreateMode);
    connect(m_editModeAction, &QAction::triggered, this, &MainWindow::setEditMode);
    connect(m_advancedModeAction, &QAction::toggled, this, &MainWindow::setAdvancedMode);
    connect(m_hideAllAction, &QAction::triggered, this, [this]() { toggleAllShapesVisible(false); });
    connect(m_showAllAction, &QAction::triggered, this, [this]() { toggleAllShapesVisible(true); });
    connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    connect(m_zoomOriginalAction, &QAction::triggered, this, &MainWindow::resetZoom);
    connect(m_fitWindowAction, &QAction::triggered, this, &MainWindow::fitWindow);
    connect(m_fitWidthAction, &QAction::triggered, this, &MainWindow::fitWidth);
    connect(m_brightenAction, &QAction::triggered, this, &MainWindow::brighten);
    connect(m_darkenAction, &QAction::triggered, this, &MainWindow::darken);
    connect(m_brightnessOriginalAction, &QAction::triggered, this, &MainWindow::resetBrightness);
    connect(m_boxLineColorAction, &QAction::triggered, this, &MainWindow::chooseBoxLineColor);
    connect(m_shapeLineColorAction, &QAction::triggered, this, &MainWindow::chooseShapeLineColor);
    connect(m_shapeFillColorAction, &QAction::triggered, this, &MainWindow::chooseShapeFillColor);
    connect(m_infoAction, &QAction::triggered, this, &MainWindow::showInfoDialog);
    connect(m_shortcutsAction, &QAction::triggered, this, &MainWindow::showShortcutsDialog);
    connect(m_drawSquareAction, &QAction::toggled, m_canvas, &Canvas::setDrawSquare);
    connect(m_displayLabelsAction, &QAction::toggled, m_canvas, &Canvas::setPaintLabels);
    connect(m_miniMapAction, &QAction::toggled, this, [this](bool checked) {
        m_miniMapOverlay->setMiniMapEnabled(checked);
        m_settings.setValue("view/miniMapEnabled", checked);
    });
    connect(m_showPerformanceAction, &QAction::toggled, this, [this](bool checked) {
        m_performanceLabel->setVisible(checked);
        m_settings.setValue("view/showPerformance", checked);
    });
    connect(m_samplingModeAction, &QAction::toggled, this, [this](bool checked) {
        m_canvas->setSamplingMode(checked ? Canvas::SamplingMode::Smooth : Canvas::SamplingMode::FastNearest);
        m_settings.setValue("view/samplingMode", checked ? QStringLiteral("Smooth") : QStringLiteral("FastNearest"));
        updatePerformanceLabel();
    });
    connect(m_useDefaultLabel, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.setValue("useDefaultLabel", checked);
    });
    connect(m_defaultLabelCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_settings.setValue("defaultLabel", text);
    });
    connect(m_difficult, &QCheckBox::toggled, this, [this](bool checked) {
        int row = m_labelList->currentRow();
        QVector<Shape> &shapes = m_canvas->shapesRef();
        if (row < 0 || row >= shapes.size()) return;
        shapes[row].difficult = checked;
        setDirty(true);
    });
    connect(m_canvas, &Canvas::selectionChanged, this, &MainWindow::onCanvasSelectionChanged);
    connect(m_canvas, &Canvas::shapesChanged, this, &MainWindow::onCanvasShapesChanged);
    connect(m_canvas, &Canvas::contextMenuRequested, this, &MainWindow::showCanvasContextMenu);
    connect(m_canvas, &Canvas::scaleChanged, this, &MainWindow::onCanvasScaleChanged);
    connect(m_canvas, &Canvas::frameRendered, this, &MainWindow::onCanvasFrameRendered);
    connect(m_canvas, &Canvas::scrollRequested, this, [this](int dx, int dy) {
        m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->value() - dx);
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->value() - dy);
    });
    connect(m_performanceMonitor, &PerformanceMonitor::performanceUpdated, this, [this](const QString &text) {
        m_resourcePerformanceText = text;
        updatePerformanceLabel();
    });
    m_miniMapOverlay->setMiniMapEnabled(m_miniMapAction->isChecked());
    m_performanceLabel->setVisible(m_showPerformanceAction->isChecked());
    m_performanceMonitor->start(1000);
    connect(m_canvas, &Canvas::statusTextChanged, m_coordinates, &QLabel::setText);
    connect(m_labelList, &QListWidget::itemSelectionChanged, this, &MainWindow::onLabelSelectionChanged);
    connect(m_labelList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { editCurrentLabel(); });
    connect(m_labelList, &QListWidget::itemChanged, this, &MainWindow::onLabelItemChanged);
    m_labelList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_labelList, &QListWidget::customContextMenuRequested, this, &MainWindow::showLabelListContextMenu);
    connect(m_fileList, &QListWidget::itemDoubleClicked, this, &MainWindow::onFileDoubleClicked);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
}

void MainWindow::loadSettings() {
    QString savedLanguage = m_settings.value("language").toString();
    if (savedLanguage.isEmpty()) {
        savedLanguage = StringBundle::systemLanguage();
        m_settings.setValue("language", savedLanguage);
    }
    m_strings = StringBundle(savedLanguage);
    m_saveDir = m_settings.value("savedir").toString();
    m_format = static_cast<SaveFormat>(m_settings.value("labelFileFormat", 0).toInt());
    m_recentFiles = m_settings.value("recentFiles").toStringList();
}

void MainWindow::saveSettings() {
    m_settings.setValue("language", m_strings.language());
    m_settings.setValue("savedir", m_saveDir);
    m_settings.setValue("labelFileFormat", static_cast<int>(m_format));
    m_settings.setValue("recentFiles", m_recentFiles);
    m_settings.setValue("filename", m_dirPath.isEmpty() ? m_filePath : QString());
    m_settings.setValue("autosave", m_autoSaveAction ? m_autoSaveAction->isChecked() : false);
    m_settings.setValue("singleclass", m_singleClassAction ? m_singleClassAction->isChecked() : false);
    m_settings.setValue("paintlabel", m_displayLabelsAction ? m_displayLabelsAction->isChecked() : false);
    m_settings.setValue("useDefaultLabel", m_useDefaultLabel ? m_useDefaultLabel->isChecked() : false);
    m_settings.setValue("defaultLabel", m_defaultLabelCombo ? m_defaultLabelCombo->currentText() : QString());
    m_settings.setValue("draw/square", m_drawSquareAction ? m_drawSquareAction->isChecked() : false);
    m_settings.setValue("view/miniMapEnabled", m_miniMapAction ? m_miniMapAction->isChecked() : true);
    m_settings.setValue("view/showPerformance", m_showPerformanceAction ? m_showPerformanceAction->isChecked() : true);
    m_settings.setValue("view/samplingMode", m_samplingModeAction && m_samplingModeAction->isChecked() ? QStringLiteral("Smooth") : QStringLiteral("FastNearest"));
    m_settings.setValue("advanced", m_advancedModeAction ? m_advancedModeAction->isChecked() : false);
    m_settings.setValue("line/color", m_lineColor);
    m_settings.setValue("fill/color", m_fillColor);
    m_settings.setValue("labelHistory", m_classList);
    m_settings.setValue("window/size", size());
    m_settings.setValue("window/position", pos());
    m_settings.setValue("window/state", saveState());
}

void MainWindow::loadPredefinedClasses() {
    loadPredefinedClassesFromFile(QDir(repoRoot()).filePath("data/predefined_classes.txt"));
    mergePersistedLabelHistory();
}

void MainWindow::loadPredefinedClassesFromFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    m_classList.clear();
    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();
        addClassLabel(line);
    }
}

void MainWindow::mergePersistedLabelHistory() {
    const QStringList persisted = m_settings.value("labelHistory").toStringList();
    for (const QString &label : persisted) {
        addClassLabel(label);
    }
}

void MainWindow::addClassLabel(const QString &label) {
    const QString text = label.trimmed();
    if (text.isEmpty() || m_classList.contains(text)) {
        return;
    }
    m_classList.append(text);
    if (m_defaultLabelCombo && m_defaultLabelCombo->findText(text) < 0) {
        m_defaultLabelCombo->addItem(text);
    }
}

bool MainWindow::loadImage(const QString &path) {
    if (!maybeSave()) return false;
    QImage image(path);
    if (image.isNull()) {
        QMessageBox::warning(this, "labelImgCpp", QString("Cannot open %1").arg(path));
        return false;
    }
    m_filePath = QFileInfo(path).absoluteFilePath();
    m_settings.setValue("lastOpenDir", QFileInfo(m_filePath).absolutePath());
    m_canvas->setPixmap(QPixmap::fromImage(image));
    loadAnnotationsForCurrentImage();
    updateFitScale();
    addRecentFile(m_filePath);
    refreshLabels();
    refreshFileListSelection();
    setDirty(false);
    setWindowTitle(QString("labelImgCpp %1").arg(m_filePath));
    updateFramelessChrome();
    m_verifyAction->setChecked(m_verified);
    return true;
}

bool MainWindow::loadAnnotation(const QString &path) {
    if (m_filePath.isEmpty() || path.isEmpty()) return false;
    AnnotationDocument doc;
    QFileInfo info(path);
    if (info.suffix().compare("xml", Qt::CaseInsensitive) == 0) {
        setFormat(SaveFormat::PascalVoc);
        if (!AnnotationIO::loadPascalVoc(path, &doc)) return false;
    } else if (info.suffix().compare("txt", Qt::CaseInsensitive) == 0) {
        setFormat(SaveFormat::Yolo);
        QImage image(m_filePath);
        if (!AnnotationIO::loadYolo(path, image.size(), &doc)) return false;
    } else if (info.suffix().compare("json", Qt::CaseInsensitive) == 0) {
        setFormat(SaveFormat::CreateMl);
        if (!AnnotationIO::loadCreateMl(path, m_filePath, &doc)) return false;
    } else {
        return false;
    }
    m_canvas->setShapes(doc.shapes);
    m_verified = doc.verified;
    m_verifyAction->setChecked(m_verified);
    refreshLabels();
    setDirty(false);
    statusBar()->showMessage(QString("Loaded annotation %1").arg(path), 5000);
    return true;
}

void MainWindow::loadAnnotationsForCurrentImage() {
    AnnotationDocument doc;
    QString dir = m_saveDir.isEmpty() ? QFileInfo(m_filePath).absolutePath() : m_saveDir;
    QString base = QDir(dir).filePath(QFileInfo(m_filePath).completeBaseName());
    if (QFileInfo::exists(base + ".xml")) {
        AnnotationIO::loadPascalVoc(base + ".xml", &doc);
    } else if (QFileInfo::exists(base + ".txt")) {
        QImage image(m_filePath);
        AnnotationIO::loadYolo(base + ".txt", image.size(), &doc);
    } else if (QFileInfo::exists(base + ".json")) {
        AnnotationIO::loadCreateMl(base + ".json", m_filePath, &doc);
    }
    m_canvas->setShapes(doc.shapes);
    m_verified = doc.verified;
}

void MainWindow::refreshLabels() {
    m_labelList->blockSignals(true);
    m_labelList->clear();
    QStringList uniqueLabels;
    const QVector<Shape> shapes = m_canvas->shapes();
    for (int i = 0; i < shapes.size(); ++i) {
        const Shape &shape = shapes[i];
        QListWidgetItem *item = new QListWidgetItem(shape.label);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        item->setCheckState(shape.visible ? Qt::Checked : Qt::Unchecked);
        item->setBackground(colorForLabel(shape.label));
        m_labelList->addItem(item);
        if (!uniqueLabels.contains(shape.label)) uniqueLabels.append(shape.label);
    }
    m_labelList->blockSignals(false);

    m_filterCombo->blockSignals(true);
    m_filterCombo->clear();
    uniqueLabels.sort();
    m_filterCombo->addItem(QString());
    m_filterCombo->addItems(uniqueLabels);
    m_filterCombo->blockSignals(false);

    if (m_canvas->currentIndex() >= 0 && m_canvas->currentIndex() < m_labelList->count()) {
        m_labelList->setCurrentRow(m_canvas->currentIndex(), QItemSelectionModel::ClearAndSelect);
    }
    refreshActions();
}

void MainWindow::refreshFileListSelection() {
    for (int i = 0; i < m_fileList->count(); ++i) {
        QListWidgetItem *item = m_fileList->item(i);
        bool selected = QFileInfo(item->text()).absoluteFilePath() == m_filePath;
        item->setSelected(selected);
        if (selected) {
            m_fileList->setCurrentItem(item);
            m_fileList->scrollToItem(item);
        }
    }
}

void MainWindow::refreshTexts() {
    m_openAction->setText(m_strings.get("openFile"));
    m_openDirAction->setText(m_strings.get("openDir"));
    m_openAnnotationAction->setText(m_strings.get("openAnnotation"));
    m_closeAction->setText(m_strings.get("closeCur"));
    m_resetAllAction->setText(m_strings.get("resetAll"));
    m_saveAction->setText(m_strings.get("save"));
    m_saveAsAction->setText(m_strings.get("saveAs"));
    m_changeSaveDirAction->setText(m_strings.get("changeSaveDir"));
    m_nextAction->setText(m_strings.get("nextImg"));
    m_prevAction->setText(m_strings.get("prevImg"));
    m_verifyAction->setText(m_strings.get("verifyImg"));
    m_editLabelAction->setText(m_strings.get("editLabel"));
    m_deleteAction->setText(m_strings.get("delBox"));
    m_copyAction->setText(m_strings.get("dupBox"));
    m_copyPreviousAction->setText(m_strings.get("copyPrevBounding"));
    m_deleteImageAction->setText(m_strings.get("deleteImg"));
    m_createModeAction->setText(m_strings.get("crtBox"));
    m_editModeAction->setText(m_strings.get("editBox"));
    m_advancedModeAction->setText(m_strings.get("advancedMode"));
    m_hideAllAction->setText(m_strings.get("hideAllBox"));
    m_showAllAction->setText(m_strings.get("showAllBox"));
    m_zoomInAction->setText(m_strings.get("zoomin"));
    m_zoomOutAction->setText(m_strings.get("zoomout"));
    m_zoomOriginalAction->setText(m_strings.get("originalsize"));
    m_fitWindowAction->setText(m_strings.get("fitWin"));
    m_fitWidthAction->setText(m_strings.get("fitWidth"));
    m_brightenAction->setText(m_strings.get("lightbrighten"));
    m_darkenAction->setText(m_strings.get("lightdarken"));
    m_brightnessOriginalAction->setText(m_strings.get("lightreset"));
    m_drawSquareAction->setText(m_strings.get("drawSquares"));
    m_boxLineColorAction->setText(m_strings.get("boxLineColor"));
    m_shapeLineColorAction->setText(m_strings.get("shapeLineColor"));
    m_shapeFillColorAction->setText(m_strings.get("shapeFillColor"));
    m_infoAction->setText(m_strings.get("info"));
    m_shortcutsAction->setText(m_strings.get("shortcut"));
    m_autoSaveAction->setText(m_strings.get("autoSaveMode"));
    m_singleClassAction->setText(m_strings.get("singleClsMode"));
    m_displayLabelsAction->setText(m_strings.get("displayLabel"));
    m_miniMapAction->setText("Mini Map");
    m_showPerformanceAction->setText("Performance");
    m_samplingModeAction->setText(m_samplingModeAction->isChecked() ? "Smooth Sampling" : "Fast Sampling");
    m_fileMenu->setTitle(m_strings.get("menu_file"));
    m_viewMenu->setTitle(m_strings.get("menu_view"));
    m_helpMenu->setTitle(m_strings.get("menu_help"));
    m_languageMenu->setTitle(m_strings.get("language"));
    m_recentFilesMenu->setTitle(m_strings.get("menu_openRecent"));
    m_useDefaultLabel->setText(m_strings.get("useDefaultLabel"));
    m_difficult->setText(m_strings.get("useDifficult"));
    m_formatAction->setText(currentFormatName());
    refreshActionToolTips();
    rebuildRecentFilesMenu();
}

void MainWindow::refreshActionToolTips() {
    const QList<QAction *> actions = {
        m_openAction,
        m_openDirAction,
        m_openAnnotationAction,
        m_closeAction,
        m_resetAllAction,
        m_saveAction,
        m_saveAsAction,
        m_changeSaveDirAction,
        m_formatAction,
        m_nextAction,
        m_prevAction,
        m_verifyAction,
        m_editLabelAction,
        m_deleteAction,
        m_copyAction,
        m_copyPreviousAction,
        m_deleteImageAction,
        m_createModeAction,
        m_editModeAction,
        m_advancedModeAction,
        m_hideAllAction,
        m_showAllAction,
        m_zoomInAction,
        m_zoomOutAction,
        m_zoomOriginalAction,
        m_fitWindowAction,
        m_fitWidthAction,
        m_brightenAction,
        m_darkenAction,
        m_brightnessOriginalAction,
        m_drawSquareAction,
        m_boxLineColorAction,
        m_shapeLineColorAction,
        m_shapeFillColorAction,
        m_infoAction,
        m_shortcutsAction,
        m_autoSaveAction,
        m_singleClassAction,
        m_displayLabelsAction,
        m_miniMapAction,
        m_showPerformanceAction,
        m_samplingModeAction,
    };
    for (QAction *action : actions) {
        if (action) {
            action->setToolTip(action->text());
        }
    }
}

void MainWindow::assignActionIcons() {
    m_openAction->setIcon(resourceIcon(QStringLiteral("open.png")));
    m_openDirAction->setIcon(resourceIcon(QStringLiteral("file.png")));
    m_openAnnotationAction->setIcon(resourceIcon(QStringLiteral("file.png")));
    m_closeAction->setIcon(resourceIcon(QStringLiteral("close.png")));
    m_resetAllAction->setIcon(resourceIcon(QStringLiteral("resetall.png")));
    m_saveAction->setIcon(resourceIcon(QStringLiteral("save.png")));
    m_saveAsAction->setIcon(resourceIcon(QStringLiteral("save-as.png")));
    m_changeSaveDirAction->setIcon(resourceIcon(QStringLiteral("save-as.png")));
    m_nextAction->setIcon(resourceIcon(QStringLiteral("next.png")));
    m_prevAction->setIcon(resourceIcon(QStringLiteral("prev.png")));
    m_verifyAction->setIcon(resourceIcon(QStringLiteral("verify.png")));
    m_editLabelAction->setIcon(resourceIcon(QStringLiteral("labels.png")));
    m_deleteAction->setIcon(resourceIcon(QStringLiteral("delete.png")));
    m_copyAction->setIcon(resourceIcon(QStringLiteral("copy.png")));
    m_copyHereAction->setIcon(resourceIcon(QStringLiteral("copy.png")));
    m_moveHereAction->setIcon(resourceIcon(QStringLiteral("edit.png")));
    m_copyPreviousAction->setIcon(resourceIcon(QStringLiteral("copy.png")));
    m_deleteImageAction->setIcon(resourceIcon(QStringLiteral("delete.png")));
    m_createModeAction->setIcon(resourceIcon(QStringLiteral("new.png")));
    m_editModeAction->setIcon(resourceIcon(QStringLiteral("edit.png")));
    m_advancedModeAction->setIcon(resourceIcon(QStringLiteral("fit.png")));
    m_hideAllAction->setIcon(resourceIcon(QStringLiteral("eye.png")));
    m_showAllAction->setIcon(resourceIcon(QStringLiteral("eye.png")));
    m_zoomInAction->setIcon(resourceIcon(QStringLiteral("zoom-in.png")));
    m_zoomOutAction->setIcon(resourceIcon(QStringLiteral("zoom-out.png")));
    m_zoomOriginalAction->setIcon(resourceIcon(QStringLiteral("zoom.png")));
    m_fitWindowAction->setIcon(resourceIcon(QStringLiteral("fit-window.png")));
    m_fitWidthAction->setIcon(resourceIcon(QStringLiteral("fit-width.png")));
    m_brightenAction->setIcon(resourceIcon(QStringLiteral("light_lighten.png")));
    m_darkenAction->setIcon(resourceIcon(QStringLiteral("light_darken.png")));
    m_brightnessOriginalAction->setIcon(resourceIcon(QStringLiteral("light_reset.png")));
    m_drawSquareAction->setIcon(resourceIcon(QStringLiteral("new.png")));
    m_boxLineColorAction->setIcon(resourceIcon(QStringLiteral("color_line.png")));
    m_shapeLineColorAction->setIcon(resourceIcon(QStringLiteral("color_line.png")));
    m_shapeFillColorAction->setIcon(resourceIcon(QStringLiteral("color.png")));
    m_infoAction->setIcon(resourceIcon(QStringLiteral("file.png")));
    m_shortcutsAction->setIcon(resourceIcon(QStringLiteral("labels.png")));
    m_autoSaveAction->setIcon(resourceIcon(QStringLiteral("save.png")));
    m_singleClassAction->setIcon(resourceIcon(QStringLiteral("labels.png")));
    m_displayLabelsAction->setIcon(resourceIcon(QStringLiteral("eye.png")));
    m_miniMapAction->setIcon(resourceIcon(QStringLiteral("fit-window.png")));
    m_showPerformanceAction->setIcon(resourceIcon(QStringLiteral("zoom.png")));
    m_samplingModeAction->setIcon(resourceIcon(QStringLiteral("fit.png")));

    switch (m_format) {
    case SaveFormat::PascalVoc:
        m_formatAction->setIcon(resourceIcon(QStringLiteral("format_voc.png")));
        break;
    case SaveFormat::Yolo:
        m_formatAction->setIcon(resourceIcon(QStringLiteral("format_yolo.png")));
        break;
    case SaveFormat::CreateMl:
        m_formatAction->setIcon(resourceIcon(QStringLiteral("format_createml.png")));
        break;
    }
}

void MainWindow::refreshActions() {
    bool hasImage = !m_filePath.isEmpty();
    bool hasSelection = m_canvas->hasSelection() || m_labelList->currentRow() >= 0;
    m_saveAction->setEnabled(hasImage);
    m_saveAsAction->setEnabled(hasImage);
    m_openAnnotationAction->setEnabled(hasImage);
    m_closeAction->setEnabled(hasImage);
    m_changeSaveDirAction->setEnabled(true);
    m_verifyAction->setEnabled(hasImage);
    m_deleteImageAction->setEnabled(hasImage);
    m_copyPreviousAction->setEnabled(hasImage && m_currentImageIndex > 0);
    m_createModeAction->setEnabled(hasImage);
    m_editModeAction->setEnabled(hasImage);
    m_advancedModeAction->setEnabled(true);
    m_editLabelAction->setEnabled(hasSelection);
    m_deleteAction->setEnabled(hasSelection);
    m_copyAction->setEnabled(hasSelection);
    m_copyHereAction->setEnabled(hasSelection);
    m_moveHereAction->setEnabled(hasSelection);
    m_hideAllAction->setEnabled(hasImage);
    m_showAllAction->setEnabled(hasImage);
    m_zoomInAction->setEnabled(hasImage);
    m_zoomOutAction->setEnabled(hasImage);
    m_zoomOriginalAction->setEnabled(hasImage);
    m_fitWindowAction->setEnabled(hasImage);
    m_fitWidthAction->setEnabled(hasImage);
    m_brightenAction->setEnabled(hasImage);
    m_darkenAction->setEnabled(hasImage);
    m_brightnessOriginalAction->setEnabled(hasImage);
    m_miniMapAction->setEnabled(true);
    m_showPerformanceAction->setEnabled(true);
    m_samplingModeAction->setEnabled(true);
    m_boxLineColorAction->setEnabled(true);
    m_shapeLineColorAction->setEnabled(hasSelection);
    m_shapeFillColorAction->setEnabled(hasSelection);
}

void MainWindow::populateToolbarForMode() {
    if (!m_toolBar) return;
    m_toolBar->clear();
    if (m_advancedModeAction && m_advancedModeAction->isChecked()) {
        m_toolBar->addActions({m_openAction, m_openDirAction, m_changeSaveDirAction, m_prevAction, m_nextAction,
                               m_saveAction, m_formatAction});
        m_toolBar->addSeparator();
        m_toolBar->addActions({m_createModeAction, m_editModeAction, m_hideAllAction, m_showAllAction});
        return;
    }
    m_toolBar->addActions({m_openAction, m_openDirAction, m_changeSaveDirAction, m_prevAction, m_nextAction,
                           m_verifyAction, m_saveAction, m_formatAction});
    m_toolBar->addSeparator();
    m_toolBar->addActions({m_createModeAction, m_editLabelAction, m_copyAction, m_deleteAction});
    m_toolBar->addSeparator();
    m_toolBar->addActions({m_zoomInAction, m_zoomOutAction, m_fitWindowAction, m_fitWidthAction,
                           m_brightenAction, m_darkenAction, m_brightnessOriginalAction});
}

void MainWindow::rebuildRecentFilesMenu() {
    if (!m_recentFilesMenu) return;
    m_recentFilesMenu->clear();
    QStringList existing;
    for (const QString &path : m_recentFiles) {
        if (QFileInfo::exists(path) && !existing.contains(path)) {
            existing.append(path);
        }
    }
    m_recentFiles = existing.mid(0, 7);
    for (int i = 0; i < m_recentFiles.size(); ++i) {
        const QString path = m_recentFiles[i];
        QAction *action = m_recentFilesMenu->addAction(QString("&%1 %2").arg(i + 1).arg(QFileInfo(path).fileName()));
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path]() { loadRecentFile(path); });
    }
    m_recentFilesMenu->setEnabled(!m_recentFiles.isEmpty());
}

void MainWindow::addRecentFile(const QString &path) {
    QString absolute = QFileInfo(path).absoluteFilePath();
    m_recentFiles.removeAll(absolute);
    m_recentFiles.prepend(absolute);
    while (m_recentFiles.size() > 7) {
        m_recentFiles.removeLast();
    }
    m_settings.setValue("recentFiles", m_recentFiles);
    rebuildRecentFilesMenu();
}

void MainWindow::loadRecentFile(const QString &path) {
    if (QFileInfo::exists(path)) {
        loadImage(path);
    } else {
        m_recentFiles.removeAll(path);
        rebuildRecentFilesMenu();
    }
}

void MainWindow::setFormat(SaveFormat format) {
    m_format = format;
    m_formatAction->setText(currentFormatName());
    assignActionIcons();
    refreshActionToolTips();
    m_settings.setValue("labelFileFormat", static_cast<int>(m_format));
}

void MainWindow::updateFitScale() {
    QSize pixmapSize = m_canvas->pixmapSize();
    if (pixmapSize.isEmpty() || m_fitMode == FitMode::Manual) {
        return;
    }
    QSize viewport = m_scrollArea->viewport()->size();
    if (m_fitMode == FitMode::Width) {
        m_canvas->setScale(qMax(0.05, (viewport.width() - 2.0) / pixmapSize.width()));
        return;
    }
    const double widthScale = (viewport.width() - 2.0) / pixmapSize.width();
    const double heightScale = (viewport.height() - 2.0) / pixmapSize.height();
    m_canvas->setScale(qMax(0.05, qMin(widthScale, heightScale)));
}

void MainWindow::openFile() {
    QString path = QFileDialog::getOpenFileName(this, m_strings.get("openFile"), m_settings.value("lastOpenDir").toString(),
                                                "Images (*.jpg *.jpeg *.png *.bmp);;All Files (*)");
    if (!path.isEmpty()) loadImage(path);
}

void MainWindow::openDir() {
    QString dir = QFileDialog::getExistingDirectory(this, m_strings.get("openDir"), m_settings.value("lastOpenDir").toString());
    if (dir.isEmpty()) return;
    m_settings.setValue("lastOpenDir", dir);
    m_dirPath = dir;
    m_imageList = scanImages(dir);
    m_fileList->clear();
    for (const QString &path : m_imageList) m_fileList->addItem(path);
    m_currentImageIndex = 0;
    if (!m_imageList.isEmpty()) loadImage(m_imageList.first());
}

void MainWindow::openAnnotationDialog() {
    if (m_filePath.isEmpty()) {
        statusBar()->showMessage("Please select image first", 5000);
        return;
    }
    QString filter;
    if (m_format == SaveFormat::PascalVoc) filter = "Pascal VOC (*.xml)";
    else if (m_format == SaveFormat::Yolo) filter = "YOLO (*.txt)";
    else filter = "CreateML (*.json)";
    QString path = QFileDialog::getOpenFileName(this, m_strings.get("openAnnotation"),
                                                QFileInfo(m_filePath).absolutePath(),
                                                filter + ";;All supported (*.xml *.txt *.json)");
    if (!path.isEmpty() && !loadAnnotation(path)) {
        QMessageBox::warning(this, "labelImgCpp", QString("Cannot open annotation %1").arg(path));
    }
}

void MainWindow::openNextImage() {
    if (m_imageList.isEmpty()) return;
    if (m_currentImageIndex + 1 < m_imageList.size()) ++m_currentImageIndex;
    loadImage(m_imageList[m_currentImageIndex]);
}

void MainWindow::openPrevImage() {
    if (m_imageList.isEmpty()) return;
    if (m_currentImageIndex > 0) --m_currentImageIndex;
    loadImage(m_imageList[m_currentImageIndex]);
}

void MainWindow::closeFile() {
    if (!maybeSave()) return;
    m_filePath.clear();
    m_verified = false;
    m_canvas->setPixmap(QPixmap());
    m_canvas->setShapes({});
    m_labelList->clear();
    m_filterCombo->clear();
    m_coordinates->clear();
    m_fileList->clearSelection();
    setDirty(false);
    setWindowTitle("labelImgCpp");
    refreshActions();
}

void MainWindow::resetAllSettings() {
    if (QMessageBox::question(this, "labelImgCpp", m_strings.get("resetAll"),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    m_settings.clear();
    saveSettings();
    QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
    close();
}

void MainWindow::saveFile() {
    if (m_filePath.isEmpty()) return;
    QString path = annotationPathForImage(m_filePath);
    AnnotationDocument doc = currentDocument();
    bool ok = false;
    if (m_format == SaveFormat::PascalVoc) ok = AnnotationIO::savePascalVoc(path, doc);
    if (m_format == SaveFormat::Yolo) ok = AnnotationIO::saveYolo(path, doc, m_classList);
    if (m_format == SaveFormat::CreateMl) ok = AnnotationIO::saveCreateMl(path, doc);
    if (ok) setDirty(false);
}

void MainWindow::saveFileAs() {
    QString path = QFileDialog::getSaveFileName(this, m_strings.get("saveAs"));
    if (path.isEmpty()) return;
    AnnotationDocument doc = currentDocument();
    bool ok = false;
    if (m_format == SaveFormat::PascalVoc) ok = AnnotationIO::savePascalVoc(path, doc);
    if (m_format == SaveFormat::Yolo) ok = AnnotationIO::saveYolo(path, doc, m_classList);
    if (m_format == SaveFormat::CreateMl) ok = AnnotationIO::saveCreateMl(path, doc);
    if (ok) setDirty(false);
}

void MainWindow::changeSaveDir() {
    QString dir = QFileDialog::getExistingDirectory(this, m_strings.get("changeSaveDir"), m_saveDir);
    if (!dir.isEmpty()) {
        m_saveDir = dir;
        m_settings.setValue("savedir", m_saveDir);
    }
}

void MainWindow::changeFormat() {
    if (m_format == SaveFormat::PascalVoc) setFormat(SaveFormat::Yolo);
    else if (m_format == SaveFormat::Yolo) setFormat(SaveFormat::CreateMl);
    else setFormat(SaveFormat::PascalVoc);
    setDirty(true);
}

void MainWindow::verifyImage() {
    m_verified = !m_verified;
    m_verifyAction->setChecked(m_verified);
    setDirty(true);
}

void MainWindow::editCurrentLabel() {
    int row = m_labelList->currentRow();
    QVector<Shape> &shapes = m_canvas->shapesRef();
    if (row < 0 || row >= shapes.size()) return;

    bool ok = false;
    QString text = QInputDialog::getItem(this, m_strings.get("editLabel"), m_strings.get("labelDialog"),
                                         m_classList, m_classList.indexOf(shapes[row].label), true, &ok);
    text = text.trimmed();
    if (!ok || text.isEmpty()) return;

    if (m_singleClassAction->isChecked()) {
        for (Shape &shape : shapes) {
            shape.label = text;
            shape.lineColor = colorForLabel(text);
        }
    } else {
        shapes[row].label = text;
        shapes[row].lineColor = colorForLabel(text);
    }
    if (!m_classList.contains(text)) {
        addClassLabel(text);
        m_settings.setValue("labelHistory", m_classList);
    }
    refreshLabels();
    setDirty(true);
    m_canvas->update();
}

void MainWindow::deleteCurrentShape() {
    int row = m_labelList->currentRow();
    if (row >= 0) m_canvas->setCurrentIndex(row);
    m_canvas->deleteCurrent();
    refreshLabels();
    setDirty(true);
}

void MainWindow::copyCurrentShape() {
    int row = m_labelList->currentRow();
    QVector<Shape> &shapes = m_canvas->shapesRef();
    if (row < 0 || row >= shapes.size()) return;
    Shape copy = shapes[row];
    copy.moveBy(QPointF(5, 5));
    shapes.push_back(copy);
    m_canvas->setCurrentIndex(shapes.size() - 1);
    refreshLabels();
    setDirty(true);
}

void MainWindow::copyPreviousBoundingBoxes() {
    if (m_currentImageIndex <= 0 || m_currentImageIndex >= m_imageList.size()) return;
    AnnotationDocument previous;
    QString previousPath = m_imageList[m_currentImageIndex - 1];
    QString base = QFileInfo(previousPath).absolutePath() + "/" + QFileInfo(previousPath).completeBaseName();
    if (QFileInfo::exists(base + ".xml")) {
        AnnotationIO::loadPascalVoc(base + ".xml", &previous);
    } else if (QFileInfo::exists(base + ".txt")) {
        QImage image(previousPath);
        AnnotationIO::loadYolo(base + ".txt", image.size(), &previous);
    } else if (QFileInfo::exists(base + ".json")) {
        AnnotationIO::loadCreateMl(base + ".json", previousPath, &previous);
    }
    if (previous.shapes.isEmpty()) return;
    QVector<Shape> shapes = m_canvas->shapes();
    shapes += previous.shapes;
    m_canvas->setShapes(shapes);
    refreshLabels();
    setDirty(true);
}

void MainWindow::deleteCurrentImage() {
    if (m_filePath.isEmpty()) return;
    if (QMessageBox::question(this, "labelImgCpp", QString("Delete image?\n%1").arg(m_filePath),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    QString deleted = m_filePath;
    QFile::moveToTrash(deleted);
    int removedIndex = m_imageList.indexOf(deleted);
    if (removedIndex >= 0) {
        m_imageList.removeAt(removedIndex);
    }
    m_fileList->clear();
    for (const QString &path : m_imageList) m_fileList->addItem(path);
    if (m_imageList.isEmpty()) {
        m_filePath.clear();
        m_canvas->setPixmap(QPixmap());
        m_canvas->setShapes({});
        refreshLabels();
        setDirty(false);
        return;
    }
    m_currentImageIndex = qMin(removedIndex, m_imageList.size() - 1);
    loadImage(m_imageList[m_currentImageIndex]);
}

void MainWindow::toggleAllShapesVisible(bool visible) {
    m_canvas->setAllShapesVisible(visible);
    refreshLabels();
    setDirty(true);
}

void MainWindow::zoomIn() {
    m_fitMode = FitMode::Manual;
    m_fitWindowAction->setChecked(false);
    m_fitWidthAction->setChecked(false);
    m_canvas->addScale(0.1);
}

void MainWindow::zoomOut() {
    m_fitMode = FitMode::Manual;
    m_fitWindowAction->setChecked(false);
    m_fitWidthAction->setChecked(false);
    m_canvas->addScale(-0.1);
}

void MainWindow::resetZoom() {
    m_fitMode = FitMode::Manual;
    m_fitWindowAction->setChecked(false);
    m_fitWidthAction->setChecked(false);
    m_canvas->setScale(1.0);
}

void MainWindow::fitWindow() {
    m_fitMode = m_fitWindowAction->isChecked() ? FitMode::Window : FitMode::Manual;
    if (m_fitMode == FitMode::Window) m_fitWidthAction->setChecked(false);
    updateFitScale();
}

void MainWindow::fitWidth() {
    m_fitMode = m_fitWidthAction->isChecked() ? FitMode::Width : FitMode::Manual;
    if (m_fitMode == FitMode::Width) m_fitWindowAction->setChecked(false);
    updateFitScale();
}

void MainWindow::brighten() {
    m_canvas->addBrightness(5);
}

void MainWindow::darken() {
    m_canvas->addBrightness(-5);
}

void MainWindow::resetBrightness() {
    m_canvas->setBrightness(50);
}

void MainWindow::showCanvasContextMenu(const QPoint &globalPosition, const QPointF &imagePosition) {
    m_lastCanvasContextImagePos = imagePosition;
    QMenu menu(this);
    menu.addAction(m_createModeAction);
    menu.addSeparator();
    menu.addAction(m_editLabelAction);
    menu.addAction(m_copyAction);
    menu.addAction(m_deleteAction);
    menu.addAction(m_shapeLineColorAction);
    menu.addAction(m_shapeFillColorAction);
    if (m_canvas->hasSelection()) {
        menu.addSeparator();
        menu.addAction(m_copyHereAction);
        menu.addAction(m_moveHereAction);
    }
    menu.addSeparator();
    menu.addAction(m_hideAllAction);
    menu.addAction(m_showAllAction);
    menu.exec(globalPosition);
}

void MainWindow::copyShapeHere() {
    if (m_canvas->copyCurrentTo(m_lastCanvasContextImagePos)) {
        refreshLabels();
        setDirty(true);
    }
}

void MainWindow::moveShapeHere() {
    if (m_canvas->moveCurrentTo(m_lastCanvasContextImagePos)) {
        refreshLabels();
        setDirty(true);
    }
}

void MainWindow::chooseBoxLineColor() {
    QColor color = QColorDialog::getColor(m_lineColor, this, m_strings.get("boxLineColor"));
    if (!color.isValid()) return;
    m_lineColor = color;
    m_canvas->setLineColor(color);
    m_settings.setValue("line/color", color);
}

void MainWindow::chooseShapeLineColor() {
    int row = m_labelList->currentRow();
    QVector<Shape> &shapes = m_canvas->shapesRef();
    if (row < 0 || row >= shapes.size()) return;
    QColor color = QColorDialog::getColor(shapes[row].lineColor, this, m_strings.get("shapeLineColor"));
    if (!color.isValid()) return;
    shapes[row].lineColor = color;
    m_canvas->update();
    setDirty(true);
}

void MainWindow::chooseShapeFillColor() {
    int row = m_labelList->currentRow();
    QVector<Shape> &shapes = m_canvas->shapesRef();
    if (row < 0 || row >= shapes.size()) return;
    QColor color = QColorDialog::getColor(shapes[row].fillColor, this, m_strings.get("shapeFillColor"));
    if (!color.isValid()) return;
    shapes[row].fillColor = color;
    m_canvas->update();
    setDirty(true);
}

void MainWindow::showInfoDialog() {
    QMessageBox::information(this, m_strings.get("info"), "labelImgCpp\nQt 6 Widgets C++ port");
}

void MainWindow::showShortcutsDialog() {
    QMessageBox::information(this, m_strings.get("shortcut"),
                             "A/D: previous/next image\nW: create mode\nCtrl+J: edit mode\nZ/C: previous/next label\nS/Delete: delete label\nSpace: verified\nCtrl+Wheel: zoom\nCtrl+Shift+Wheel: brightness");
}

void MainWindow::setAdvancedMode(bool enabled) {
    if (m_advancedModeAction->isChecked() != enabled) {
        QSignalBlocker blocker(m_advancedModeAction);
        m_advancedModeAction->setChecked(enabled);
    }
    populateToolbarForMode();
    if (enabled) {
        setEditMode();
        if (m_labelDock) m_labelDock->setFeatures(m_labelDock->features() | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    } else {
        setEditMode();
        if (m_labelDock) m_labelDock->setFeatures(QDockWidget::DockWidgetClosable);
    }
    m_settings.setValue("advanced", enabled);
}

void MainWindow::setCreateMode() {
    {
        QSignalBlocker createBlocker(m_createModeAction);
        QSignalBlocker editBlocker(m_editModeAction);
        m_createModeAction->setChecked(true);
        m_editModeAction->setChecked(false);
    }
    m_canvas->setCreateMode(true);
    refreshActions();
}

void MainWindow::setEditMode() {
    {
        QSignalBlocker createBlocker(m_createModeAction);
        QSignalBlocker editBlocker(m_editModeAction);
        m_createModeAction->setChecked(false);
        m_editModeAction->setChecked(true);
    }
    m_canvas->setEditMode();
    refreshActions();
}

void MainWindow::showLabelListContextMenu(const QPoint &position) {
    QMenu menu(this);
    menu.addAction(m_editLabelAction);
    menu.addAction(m_deleteAction);
    menu.exec(m_labelList->mapToGlobal(position));
}

void MainWindow::onCanvasScaleChanged(double oldScale, double newScale, const QPoint &widgetPosition) {
    if (oldScale <= 0.0 || qFuzzyCompare(oldScale, newScale)) {
        return;
    }
    QScrollBar *hBar = m_scrollArea->horizontalScrollBar();
    QScrollBar *vBar = m_scrollArea->verticalScrollBar();
    const QPointF imagePoint(widgetPosition.x() / oldScale, widgetPosition.y() / oldScale);
    const QPointF newWidgetPoint(imagePoint.x() * newScale, imagePoint.y() * newScale);
    hBar->setValue(hBar->value() + qRound(newWidgetPoint.x() - widgetPosition.x()));
    vBar->setValue(vBar->value() + qRound(newWidgetPoint.y() - widgetPosition.y()));
    if (m_miniMapOverlay) {
        m_miniMapOverlay->refreshGeometry();
    }
    updatePerformanceLabel();
}

void MainWindow::onCanvasFrameRendered(double frameMs) {
    m_lastFrameMs = frameMs;
    updatePerformanceLabel();
}

void MainWindow::startSystemMove() {
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (hwnd) {
        ReleaseCapture();
        SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return;
    }
#endif
    if (QWindow *handle = windowHandle()) {
        handle->startSystemMove();
    }
}

void MainWindow::toggleMaximizeRestore() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    updateFramelessChrome();
}

void MainWindow::updateFramelessChrome() {
    if (!m_titleBar) {
        return;
    }
    m_titleBar->setMaximized(isMaximized() || isFullScreen());
    m_titleBar->setTitle(windowTitle().isEmpty() ? QStringLiteral("labelImgCpp") : windowTitle());
}

void MainWindow::onCanvasSelectionChanged(int index) {
    if (m_noSelectionSlot) {
        m_noSelectionSlot = false;
        return;
    }
    if (index >= 0 && index < m_labelList->count()) {
        m_noSelectionSlot = true;
        m_labelList->setCurrentRow(index, QItemSelectionModel::ClearAndSelect);
        const QVector<Shape> shapes = m_canvas->shapes();
        if (index < shapes.size()) {
            QSignalBlocker blocker(m_difficult);
            m_difficult->setChecked(shapes[index].difficult);
        }
    }
    refreshActions();
}

void MainWindow::onCanvasShapesChanged() {
    QVector<Shape> &shapes = m_canvas->shapesRef();
    bool changed = false;
    for (Shape &shape : shapes) {
        if (!shape.label.isEmpty()) {
            continue;
        }
        QString label;
        if (m_useDefaultLabel->isChecked() && m_defaultLabelCombo->currentIndex() >= 0) {
            label = m_defaultLabelCombo->currentText();
        }
        if (label.isEmpty() && !m_classList.isEmpty()) {
            label = m_classList.first();
        }
        if (label.isEmpty()) {
            label = "object";
        }
        shape.label = label;
        shape.lineColor = colorForLabel(label);
        shape.difficult = m_difficult->isChecked();
        changed = true;
    }
    if (changed) {
        refreshLabels();
    }
    setDirty(true);
}

void MainWindow::onLabelSelectionChanged() {
    if (m_noSelectionSlot) {
        m_noSelectionSlot = false;
        return;
    }
    int row = m_labelList->currentRow();
    if (row >= 0) {
        m_canvas->setCurrentIndex(row);
        const QVector<Shape> shapes = m_canvas->shapes();
        if (row < shapes.size()) {
            QSignalBlocker blocker(m_difficult);
            m_difficult->setChecked(shapes[row].difficult);
        }
    }
    refreshActions();
}

void MainWindow::onLabelItemChanged(QListWidgetItem *item) {
    int row = m_labelList->row(item);
    QVector<Shape> &shapes = m_canvas->shapesRef();
    if (row < 0 || row >= shapes.size()) return;
    const QString text = item->text().trimmed();
    const bool labelChanged = shapes[row].label != text;
    shapes[row].visible = item->checkState() == Qt::Checked;
    shapes[row].label = text;
    shapes[row].lineColor = colorForLabel(text);
    if (labelChanged) {
        addClassLabel(text);
        m_settings.setValue("labelHistory", m_classList);
    }
    setDirty(true);
    if (labelChanged) {
        refreshLabels();
    }
    m_canvas->update();
}

void MainWindow::onFileDoubleClicked(QListWidgetItem *item) {
    int index = m_imageList.indexOf(item->text());
    if (index >= 0) m_currentImageIndex = index;
    loadImage(item->text());
}

void MainWindow::onFilterChanged(int index) {
    QString text = m_filterCombo->itemText(index);
    for (int i = 0; i < m_labelList->count(); ++i) {
        QListWidgetItem *item = m_labelList->item(i);
        item->setCheckState(text.isEmpty() || item->text() == text ? Qt::Checked : Qt::Unchecked);
    }
}

void MainWindow::changeLanguage(const QString &language) {
    m_strings = StringBundle(language);
    m_settings.setValue("language", m_strings.language());
    for (QAction *action : m_languageMenu->actions()) {
        action->setChecked(action->data().toString() == m_strings.language());
    }
    refreshTexts();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateFitScale();
    updateFramelessChrome();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Control) {
        m_canvas->setDrawSquare(true);
    } else if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_W) {
        setCreateMode();
    } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_J) {
        setEditMode();
    } else if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Z) {
        selectAdjacentLabel(-1);
    } else if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_C) {
        selectAdjacentLabel(1);
    } else if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_S) {
        deleteCurrentShape();
    } else if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Q) {
        toggleAllShapesVisible(false);
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Control) {
        m_canvas->setDrawSquare(m_drawSquareAction->isChecked());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        updateFramelessChrome();
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG") {
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_NCCALCSIZE && msg->wParam == TRUE) {
        *result = 0;
        return true;
    }

    if (msg->message != WM_NCHITTEST || isMaximized() || isFullScreen() || !m_titleBar) {
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    const int border = 8;
    const QPoint globalPos(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
    const QRect titleFrame(m_titleBar->mapToGlobal(QPoint(0, 0)), m_titleBar->size());
    const HitRegion region = hitRegionFor(frameGeometry(), titleFrame, globalPos, border);

    switch (region) {
    case HitRegion::TopLeft:
        *result = HTTOPLEFT;
        return true;
    case HitRegion::TopRight:
        *result = HTTOPRIGHT;
        return true;
    case HitRegion::BottomLeft:
        *result = HTBOTTOMLEFT;
        return true;
    case HitRegion::BottomRight:
        *result = HTBOTTOMRIGHT;
        return true;
    case HitRegion::Left:
        *result = HTLEFT;
        return true;
    case HitRegion::Right:
        *result = HTRIGHT;
        return true;
    case HitRegion::Top:
        *result = HTTOP;
        return true;
    case HitRegion::Bottom:
        *result = HTBOTTOM;
        return true;
    case HitRegion::Caption: {
        QWidget *child = childAt(mapFromGlobal(globalPos));
        if (child && (qobject_cast<QToolButton *>(child) || qobject_cast<QToolButton *>(child->parentWidget()))) {
            break;
        }
        *result = HTCAPTION;
        return true;
    }
    case HitRegion::Client:
        break;
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::selectLabelRow(int row) {
    if (m_labelList->count() == 0) return;
    row = qBound(0, row, m_labelList->count() - 1);
    m_labelList->setCurrentRow(row, QItemSelectionModel::ClearAndSelect);
    m_canvas->setCurrentIndex(row);
}

void MainWindow::selectAdjacentLabel(int step) {
    if (m_labelList->count() == 0) return;
    int row = m_labelList->currentRow();
    if (row < 0) {
        selectLabelRow(0);
        return;
    }
    int next = (row + step) % m_labelList->count();
    if (next < 0) next += m_labelList->count();
    selectLabelRow(next);
}

QString MainWindow::annotationPathForImage(const QString &imagePath) const {
    QString dir = m_saveDir.isEmpty() ? QFileInfo(imagePath).absolutePath() : m_saveDir;
    QString base = QFileInfo(imagePath).completeBaseName();
    if (m_format == SaveFormat::PascalVoc) return QDir(dir).filePath(base + ".xml");
    if (m_format == SaveFormat::Yolo) return QDir(dir).filePath(base + ".txt");
    return QDir(dir).filePath(base + ".json");
}

AnnotationDocument MainWindow::currentDocument() const {
    AnnotationDocument doc;
    doc.imagePath = m_filePath;
    QImage image(m_filePath);
    doc.imageSize = image.size();
    doc.depth = image.isGrayscale() ? 1 : 3;
    doc.verified = m_verified;
    doc.shapes = m_canvas->shapes();
    return doc;
}

void MainWindow::setDirty(bool dirty) {
    m_dirty = dirty;
    refreshActions();
}

bool MainWindow::maybeSave() {
    if (!m_dirty) return true;
    if (m_autoSaveAction && m_autoSaveAction->isChecked()) {
        saveFile();
        return true;
    }
    QMessageBox::StandardButton result = QMessageBox::question(this, "labelImgCpp", "Save changes?",
                                                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (result == QMessageBox::Cancel) return false;
    if (result == QMessageBox::Yes) saveFile();
    return true;
}

QStringList MainWindow::scanImages(const QString &dirPath) const {
    QStringList result;
    QDirIterator it(dirPath, supportedImageNameFilters(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) result.append(QFileInfo(it.next()).absoluteFilePath());
    std::sort(result.begin(), result.end(), naturalPathLess);
    return result;
}

QString MainWindow::currentFormatName() const {
    if (m_format == SaveFormat::PascalVoc) return "PascalVOC";
    if (m_format == SaveFormat::Yolo) return "YOLO";
    return "CreateML";
}

void MainWindow::updatePerformanceLabel() {
    if (!m_performanceLabel || !m_canvas) {
        return;
    }
    const double fps = m_lastFrameMs > 0.0 ? 1000.0 / m_lastFrameMs : 0.0;
    const QString sampling = m_canvas->samplingMode() == Canvas::SamplingMode::Smooth ? "Smooth" : "Fast";
    m_performanceLabel->setText(QStringLiteral("%1 | FPS %2 | Scale %3% | %4")
                                    .arg(m_resourcePerformanceText)
                                    .arg(fps, 0, 'f', 1)
                                    .arg(m_canvas->scale() * 100.0, 0, 'f', 1)
                                    .arg(sampling));
    if (m_samplingModeAction) {
        m_samplingModeAction->setText(m_samplingModeAction->isChecked() ? "Smooth Sampling" : "Fast Sampling");
    }
}
