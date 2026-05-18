#include "ui/MiniMapOverlay.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

namespace {
constexpr int kMargin = 10;
constexpr int kMaxSide = 200;
constexpr int kMinSide = 100;
}

MiniMapOverlay::MiniMapOverlay(Canvas *canvas, QScrollArea *scrollArea, QWidget *parent)
    : QWidget(parent), m_canvas(canvas), m_scrollArea(scrollArea) {
    setMouseTracking(true);
    setAttribute(Qt::WA_StyledBackground, false);
    setCursor(Qt::PointingHandCursor);
    if (parent) {
        parent->installEventFilter(this);
    }
    if (m_scrollArea) {
        connect(m_scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, QOverload<>::of(&MiniMapOverlay::update));
        connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, QOverload<>::of(&MiniMapOverlay::update));
    }
    if (m_canvas) {
        connect(m_canvas, &Canvas::frameRendered, this, [this]() {
            refreshGeometry();
            update();
        });
    }
    refreshGeometry();
}

void MiniMapOverlay::setMiniMapEnabled(bool enabled) {
    m_enabled = enabled;
    updateVisibility();
}

bool MiniMapOverlay::miniMapEnabled() const {
    return m_enabled;
}

void MiniMapOverlay::refreshGeometry() {
    if (!m_canvas || !m_scrollArea || !parentWidget()) {
        hide();
        return;
    }

    m_overview = m_canvas->overviewImage(kMaxSide);
    if (m_overview.isNull()) {
        updateVisibility();
        return;
    }

    QSize size = m_overview.size();
    if (qMax(size.width(), size.height()) < kMinSide) {
        size.scale(QSize(kMinSide, kMinSide), Qt::KeepAspectRatio);
    }

    QWidget *viewport = parentWidget();
    const int x = viewport->width() - size.width() - kMargin;
    const int y = viewport->height() - size.height() - kMargin;
    setGeometry(QRect(QPoint(qMax(kMargin, x), qMax(kMargin, y)), size));
    raise();
    updateVisibility();
}

bool MiniMapOverlay::eventFilter(QObject *watched, QEvent *event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        refreshGeometry();
    }
    return QWidget::eventFilter(watched, event);
}

void MiniMapOverlay::paintEvent(QPaintEvent *) {
    if (!m_canvas || m_overview.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const double alpha = (m_hovered || m_dragging) ? 1.0 : 0.85;

    painter.fillRect(rect(), QColor(40, 40, 40, static_cast<int>(200 * alpha)));
    painter.setOpacity(alpha);
    painter.drawImage(rect(), m_overview);
    painter.setOpacity(1.0);

    painter.setPen(QPen(QColor(100, 100, 100), 2));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    QRectF visible = viewportRectOnMiniMap();
    if (!visible.isEmpty()) {
        painter.setPen(QPen(QColor(255, 80, 80), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(visible.adjusted(1, 1, -1, -1));
    }
}

void MiniMapOverlay::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        centerViewOnMiniMapPoint(event->position());
        update();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MiniMapOverlay::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) {
        centerViewOnMiniMapPoint(event->position());
        update();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MiniMapOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        update();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MiniMapOverlay::enterEvent(QEnterEvent *event) {
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void MiniMapOverlay::leaveEvent(QEvent *event) {
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

QRectF MiniMapOverlay::viewportRectOnMiniMap() const {
    if (!m_canvas || !m_scrollArea || m_canvas->pixmapSize().isEmpty()) {
        return {};
    }

    const QSize imageSize = m_canvas->pixmapSize();
    const double xScale = static_cast<double>(width()) / imageSize.width();
    const double yScale = static_cast<double>(height()) / imageSize.height();
    const QRectF visibleImageRect(QPointF(m_scrollArea->horizontalScrollBar()->value() / m_canvas->scale(),
                                          m_scrollArea->verticalScrollBar()->value() / m_canvas->scale()),
                                  QSizeF(m_scrollArea->viewport()->width() / m_canvas->scale(),
                                         m_scrollArea->viewport()->height() / m_canvas->scale()));
    QRectF mapped(visibleImageRect.x() * xScale,
                  visibleImageRect.y() * yScale,
                  visibleImageRect.width() * xScale,
                  visibleImageRect.height() * yScale);
    return mapped.intersected(rect());
}

void MiniMapOverlay::centerViewOnMiniMapPoint(const QPointF &point) {
    if (!m_canvas || !m_scrollArea || m_canvas->pixmapSize().isEmpty()) {
        return;
    }

    const QPointF clamped(qBound(0.0, point.x(), static_cast<double>(width())),
                          qBound(0.0, point.y(), static_cast<double>(height())));
    const QSize imageSize = m_canvas->pixmapSize();
    const QPointF imagePoint(clamped.x() / width() * imageSize.width(),
                             clamped.y() / height() * imageSize.height());
    QScrollBar *hBar = m_scrollArea->horizontalScrollBar();
    QScrollBar *vBar = m_scrollArea->verticalScrollBar();
    hBar->setValue(qRound(imagePoint.x() * m_canvas->scale() - m_scrollArea->viewport()->width() / 2.0));
    vBar->setValue(qRound(imagePoint.y() * m_canvas->scale() - m_scrollArea->viewport()->height() / 2.0));
}

void MiniMapOverlay::updateVisibility() {
    const bool shouldShow = m_enabled && !m_overview.isNull();
    setVisible(shouldShow);
    if (shouldShow) {
        raise();
    }
}
