#pragma once

#include "core/Shape.h"

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QPixmap>
#include <QRectF>
#include <QWidget>
#include <QVector>

class Canvas : public QWidget {
    Q_OBJECT

public:
    enum class SamplingMode { FastNearest, Smooth };

    explicit Canvas(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap);
    void setShapes(const QVector<Shape> &shapes);
    QVector<Shape> shapes() const;
    QVector<Shape> &shapesRef();
    QSize pixmapSize() const;
    void setScale(double scale);
    void addScale(double delta);
    double scale() const;
    void setSamplingMode(SamplingMode mode);
    SamplingMode samplingMode() const;
    QImage overviewImage(int maxSide) const;
    void setBrightness(int brightness);
    void addBrightness(int delta);
    int brightness() const;
    void setCurrentIndex(int index);
    int currentIndex() const;
    void setDrawSquare(bool enabled);
    void setCreateMode(bool enabled);
    void setEditing(bool enabled);
    void setEditMode();
    void cancelDrawing();
    void setLineColor(const QColor &color);
    void setFillColor(const QColor &color);
    void setPaintLabels(bool enabled);
    void setAllShapesVisible(bool visible);
    bool hasSelection() const;
    void deleteCurrent();
    bool copyCurrentTo(const QPointF &imagePoint);
    bool moveCurrentTo(const QPointF &imagePoint);

signals:
    void shapesChanged();
    void selectionChanged(int index);
    void scrollRequested(int dx, int dy);
    void statusTextChanged(const QString &text);
    void contextMenuRequested(const QPoint &globalPosition, const QPointF &imagePosition);
    void scaleChanged(double oldScale, double newScale, const QPoint &widgetPosition);
    void frameRendered(double frameMs);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPointF imagePos(const QPointF &widgetPos) const;
    QRectF widgetRect(const Shape &shape) const;
    int shapeAt(const QPointF &imagePoint) const;
    void selectIndex(int index);
    QPointF boundedTopLeftForCenter(const Shape &shape, const QPointF &center) const;
    QPointF boundedDeltaForMove(const Shape &shape, const QPointF &delta) const;
    QPointF clampToPixmap(const QPointF &point) const;

    QPixmap m_pixmap;
    QImage m_previewImage;
    QVector<Shape> m_shapes;
    int m_currentIndex = -1;
    double m_scale = 1.0;
    int m_brightness = 50;
    SamplingMode m_samplingMode = SamplingMode::FastNearest;
    QColor m_lineColor = QColor(0, 255, 0, 128);
    QColor m_fillColor = QColor(255, 0, 0, 128);
    bool m_drawSquare = false;
    bool m_createMode = true;
    bool m_editing = true;
    bool m_paintLabels = false;
    bool m_creating = false;
    bool m_moving = false;
    bool m_movingVertex = false;
    bool m_rightPanning = false;
    bool m_rightDragged = false;
    int m_hoverVertex = -1;
    int m_hoverShape = -1;
    QPoint m_lastMousePos;
    QPoint m_rightPressPos;
    QPointF m_startImagePos;
};
