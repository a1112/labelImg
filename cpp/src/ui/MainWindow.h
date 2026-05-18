#pragma once

#include "core/AnnotationIO.h"
#include "core/PerformanceMonitor.h"
#include "core/StringBundle.h"
#include "ui/Canvas.h"
#include "ui/FramelessTitleBar.h"
#include "ui/MiniMapOverlay.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QScrollArea>
#include <QSettings>
#include <QToolBar>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void loadStartupArgs(const QStringList &arguments);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void openFile();
    void openDir();
    void openAnnotationDialog();
    void openNextImage();
    void openPrevImage();
    void closeFile();
    void resetAllSettings();
    void saveFile();
    void saveFileAs();
    void changeSaveDir();
    void changeFormat();
    void verifyImage();
    void editCurrentLabel();
    void deleteCurrentShape();
    void copyCurrentShape();
    void copyPreviousBoundingBoxes();
    void deleteCurrentImage();
    void toggleAllShapesVisible(bool visible);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitWindow();
    void fitWidth();
    void brighten();
    void darken();
    void resetBrightness();
    void showCanvasContextMenu(const QPoint &globalPosition, const QPointF &imagePosition);
    void copyShapeHere();
    void moveShapeHere();
    void chooseBoxLineColor();
    void chooseShapeLineColor();
    void chooseShapeFillColor();
    void showInfoDialog();
    void showShortcutsDialog();
    void setAdvancedMode(bool enabled);
    void setCreateMode();
    void setEditMode();
    void showLabelListContextMenu(const QPoint &position);
    void onCanvasScaleChanged(double oldScale, double newScale, const QPoint &widgetPosition);
    void onCanvasFrameRendered(double frameMs);
    void onCanvasSelectionChanged(int index);
    void onCanvasShapesChanged();
    void onLabelSelectionChanged();
    void onLabelItemChanged(QListWidgetItem *item);
    void onFileDoubleClicked(QListWidgetItem *item);
    void onFilterChanged(int index);
    void changeLanguage(const QString &language);

private:
    enum class SaveFormat { PascalVoc, Yolo, CreateMl };

    void createUi();
    void createActions();
    void createMenusAndToolbars();
    void connectSignals();
    void installFramelessChrome();
    void startSystemMove();
    void toggleMaximizeRestore();
    void updateFramelessChrome();
    void loadSettings();
    void saveSettings();
    void loadPredefinedClasses();
    void loadPredefinedClassesFromFile(const QString &path);
    void mergePersistedLabelHistory();
    void addClassLabel(const QString &label);
    bool loadImage(const QString &path);
    bool loadAnnotation(const QString &path);
    void loadAnnotationsForCurrentImage();
    void refreshLabels();
    void refreshFileListSelection();
    void refreshTexts();
    void refreshActionToolTips();
    void assignActionIcons();
    void refreshActions();
    void populateToolbarForMode();
    void rebuildRecentFilesMenu();
    void addRecentFile(const QString &path);
    void loadRecentFile(const QString &path);
    void setFormat(SaveFormat format);
    void updateFitScale();
    void selectLabelRow(int row);
    void selectAdjacentLabel(int step);
    QString annotationPathForImage(const QString &imagePath) const;
    AnnotationDocument currentDocument() const;
    void setDirty(bool dirty);
    bool maybeSave();
    QStringList scanImages(const QString &dirPath) const;
    QString currentFormatName() const;
    void updatePerformanceLabel();

    StringBundle m_strings;
    QSettings m_settings;
    Canvas *m_canvas = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QDockWidget *m_labelDock = nullptr;
    QDockWidget *m_fileDock = nullptr;
    QListWidget *m_labelList = nullptr;
    QListWidget *m_fileList = nullptr;
    QComboBox *m_filterCombo = nullptr;
    QComboBox *m_defaultLabelCombo = nullptr;
    QCheckBox *m_useDefaultLabel = nullptr;
    QCheckBox *m_difficult = nullptr;
    QLabel *m_coordinates = nullptr;
    QLabel *m_performanceLabel = nullptr;
    MiniMapOverlay *m_miniMapOverlay = nullptr;
    PerformanceMonitor *m_performanceMonitor = nullptr;
    FramelessTitleBar *m_titleBar = nullptr;
    QWidget *m_topChrome = nullptr;

    QMenu *m_fileMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_helpMenu = nullptr;
    QMenu *m_languageMenu = nullptr;
    QMenu *m_recentFilesMenu = nullptr;
    QToolBar *m_toolBar = nullptr;

    QAction *m_openAction = nullptr;
    QAction *m_openDirAction = nullptr;
    QAction *m_openAnnotationAction = nullptr;
    QAction *m_closeAction = nullptr;
    QAction *m_resetAllAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAsAction = nullptr;
    QAction *m_changeSaveDirAction = nullptr;
    QAction *m_formatAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_prevAction = nullptr;
    QAction *m_verifyAction = nullptr;
    QAction *m_editLabelAction = nullptr;
    QAction *m_deleteAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_copyHereAction = nullptr;
    QAction *m_moveHereAction = nullptr;
    QAction *m_copyPreviousAction = nullptr;
    QAction *m_deleteImageAction = nullptr;
    QAction *m_createModeAction = nullptr;
    QAction *m_editModeAction = nullptr;
    QAction *m_advancedModeAction = nullptr;
    QAction *m_hideAllAction = nullptr;
    QAction *m_showAllAction = nullptr;
    QAction *m_zoomInAction = nullptr;
    QAction *m_zoomOutAction = nullptr;
    QAction *m_zoomOriginalAction = nullptr;
    QAction *m_fitWindowAction = nullptr;
    QAction *m_fitWidthAction = nullptr;
    QAction *m_brightenAction = nullptr;
    QAction *m_darkenAction = nullptr;
    QAction *m_brightnessOriginalAction = nullptr;
    QAction *m_drawSquareAction = nullptr;
    QAction *m_boxLineColorAction = nullptr;
    QAction *m_shapeLineColorAction = nullptr;
    QAction *m_shapeFillColorAction = nullptr;
    QAction *m_infoAction = nullptr;
    QAction *m_shortcutsAction = nullptr;
    QAction *m_autoSaveAction = nullptr;
    QAction *m_singleClassAction = nullptr;
    QAction *m_displayLabelsAction = nullptr;
    QAction *m_miniMapAction = nullptr;
    QAction *m_showPerformanceAction = nullptr;
    QAction *m_samplingModeAction = nullptr;

    QStringList m_classList;
    QStringList m_imageList;
    QStringList m_recentFiles;
    QString m_filePath;
    QString m_dirPath;
    QString m_saveDir;
    int m_currentImageIndex = 0;
    SaveFormat m_format = SaveFormat::PascalVoc;
    enum class FitMode { Manual, Window, Width };
    FitMode m_fitMode = FitMode::Manual;
    QColor m_lineColor = QColor(0, 255, 0, 128);
    QColor m_fillColor = QColor(255, 0, 0, 128);
    QPointF m_lastCanvasContextImagePos;
    QString m_resourcePerformanceText = "CPU 0.0% | MEM 0 MB";
    double m_lastFrameMs = 0.0;
    bool m_dirty = false;
    bool m_verified = false;
    bool m_noSelectionSlot = false;
};
