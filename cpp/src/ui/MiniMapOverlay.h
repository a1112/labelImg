#pragma once

#include "ui/Canvas.h"

#include <QPointer>
#include <QScrollArea>
#include <QWidget>

class MiniMapOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MiniMapOverlay(Canvas *canvas, QScrollArea *scrollArea, QWidget *parent = nullptr);

    void setMiniMapEnabled(bool enabled);
    bool miniMapEnabled() const;
    void refreshGeometry();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF viewportRectOnMiniMap() const;
    void centerViewOnMiniMapPoint(const QPointF &point);
    void updateVisibility();

    QPointer<Canvas> m_canvas;
    QPointer<QScrollArea> m_scrollArea;
    bool m_enabled = true;
    bool m_hovered = false;
    bool m_dragging = false;
    QImage m_overview;
};
