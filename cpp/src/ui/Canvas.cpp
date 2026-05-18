#include "ui/Canvas.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QWheelEvent>

Canvas::Canvas(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);
}

namespace {
QSize scaledSize(const QPixmap &pixmap, double scale) {
    return QSize(qMax(1, qRound(pixmap.width() * scale)),
                 qMax(1, qRound(pixmap.height() * scale)));
}

bool isTooSmallToKeep(const QRectF &rect) {
    constexpr qreal minimumBoxSize = 3.0;
    return rect.width() < minimumBoxSize || rect.height() < minimumBoxSize;
}
}

void Canvas::setPixmap(const QPixmap &pixmap) {
    m_pixmap = pixmap;
    m_previewImage = overviewImage(1600);
    setMinimumSize(scaledSize(m_pixmap, m_scale));
    updateGeometry();
    update();
}

void Canvas::setShapes(const QVector<Shape> &shapes) {
    m_shapes = shapes;
    m_currentIndex = m_shapes.isEmpty() ? -1 : m_shapes.size() - 1;
    update();
}

QVector<Shape> Canvas::shapes() const {
    return m_shapes;
}

QVector<Shape> &Canvas::shapesRef() {
    return m_shapes;
}

QSize Canvas::pixmapSize() const {
    return m_pixmap.size();
}

void Canvas::setScale(double scale) {
    m_scale = qBound(0.005, scale, 16.0);
    setMinimumSize(scaledSize(m_pixmap, m_scale));
    updateGeometry();
    update();
}

void Canvas::addScale(double delta) {
    setScale(m_scale + delta);
}

double Canvas::scale() const {
    return m_scale;
}

void Canvas::setSamplingMode(SamplingMode mode) {
    if (m_samplingMode == mode) {
        return;
    }
    m_samplingMode = mode;
    update();
}

Canvas::SamplingMode Canvas::samplingMode() const {
    return m_samplingMode;
}

QImage Canvas::overviewImage(int maxSide) const {
    if (m_pixmap.isNull() || maxSide <= 0) {
        return {};
    }
    const QSize sourceSize = m_pixmap.size();
    const double scale = qMin(static_cast<double>(maxSide) / sourceSize.width(),
                              static_cast<double>(maxSide) / sourceSize.height());
    QSize targetSize(qMax(1, qRound(sourceSize.width() * scale)),
                     qMax(1, qRound(sourceSize.height() * scale)));
    return m_pixmap.toImage().scaled(targetSize, Qt::KeepAspectRatio, Qt::FastTransformation);
}

void Canvas::setBrightness(int brightness) {
    m_brightness = qBound(0, brightness, 100);
    update();
}

void Canvas::addBrightness(int delta) {
    setBrightness(m_brightness + delta);
}

int Canvas::brightness() const {
    return m_brightness;
}

void Canvas::setCurrentIndex(int index) {
    selectIndex(index);
}

int Canvas::currentIndex() const {
    return m_currentIndex;
}

void Canvas::setDrawSquare(bool enabled) {
    m_drawSquare = enabled;
}

void Canvas::setCreateMode(bool enabled) {
    m_createMode = enabled;
    m_editing = !enabled;
    if (enabled) {
        setCursor(Qt::CrossCursor);
    } else {
        unsetCursor();
    }
}

void Canvas::setEditing(bool enabled) {
    m_editing = enabled;
    m_createMode = !enabled;
    if (enabled) {
        unsetCursor();
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void Canvas::setEditMode() {
    setEditing(true);
}

void Canvas::cancelDrawing() {
    if (m_creating && hasSelection()) {
        m_shapes.removeAt(m_currentIndex);
        m_currentIndex = -1;
        m_creating = false;
        emit shapesChanged();
        emit selectionChanged(m_currentIndex);
        update();
    }
}

void Canvas::setLineColor(const QColor &color) {
    m_lineColor = color;
}

void Canvas::setFillColor(const QColor &color) {
    m_fillColor = color;
}

void Canvas::setPaintLabels(bool enabled) {
    m_paintLabels = enabled;
    for (Shape &shape : m_shapes) {
        shape.paintLabel = enabled;
    }
    update();
}

void Canvas::setAllShapesVisible(bool visible) {
    for (Shape &shape : m_shapes) {
        shape.visible = visible;
    }
    emit shapesChanged();
    update();
}

bool Canvas::hasSelection() const {
    return m_currentIndex >= 0 && m_currentIndex < m_shapes.size();
}

void Canvas::deleteCurrent() {
    if (!hasSelection()) {
        return;
    }
    m_shapes.removeAt(m_currentIndex);
    if (m_shapes.isEmpty()) {
        m_currentIndex = -1;
    } else if (m_currentIndex >= m_shapes.size()) {
        m_currentIndex = 0;
    }
    emit shapesChanged();
    emit selectionChanged(m_currentIndex);
    update();
}

bool Canvas::copyCurrentTo(const QPointF &imagePoint) {
    if (!hasSelection()) {
        return false;
    }
    Shape copy = m_shapes[m_currentIndex].copy();
    QPointF topLeft = boundedTopLeftForCenter(copy, imagePoint);
    copy.moveBy(topLeft - copy.boundingRect().topLeft());
    m_shapes.push_back(copy);
    selectIndex(m_shapes.size() - 1);
    emit shapesChanged();
    update();
    return true;
}

bool Canvas::moveCurrentTo(const QPointF &imagePoint) {
    if (!hasSelection()) {
        return false;
    }
    QPointF topLeft = boundedTopLeftForCenter(m_shapes[m_currentIndex], imagePoint);
    m_shapes[m_currentIndex].moveBy(topLeft - m_shapes[m_currentIndex].boundingRect().topLeft());
    emit shapesChanged();
    emit selectionChanged(m_currentIndex);
    update();
    return true;
}

void Canvas::paintEvent(QPaintEvent *) {
    QElapsedTimer frameTimer;
    frameTimer.start();
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    if (m_pixmap.isNull()) {
        return;
    }

    painter.save();
    painter.scale(m_scale, m_scale);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_samplingMode == SamplingMode::Smooth);
    if (m_samplingMode == SamplingMode::FastNearest && m_scale < 0.25 && !m_previewImage.isNull()) {
        painter.drawImage(QRectF(QPointF(0, 0), m_pixmap.size()), m_previewImage);
    } else {
        painter.drawPixmap(0, 0, m_pixmap);
    }
    if (m_brightness != 50) {
        QColor overlay = m_brightness > 50 ? QColor(255, 255, 255, (m_brightness - 50) * 4)
                                           : QColor(0, 0, 0, (50 - m_brightness) * 4);
        painter.fillRect(QRectF(QPointF(0, 0), m_pixmap.size()), overlay);
    }

    for (int i = 0; i < m_shapes.size(); ++i) {
        const Shape &shape = m_shapes[i];
        if (!shape.visible) {
            continue;
        }
        QRectF box = shape.boundingRect();
        QPen pen(i == m_currentIndex ? Qt::white : shape.lineColor, i == m_currentIndex ? 2.0 / m_scale : 1.5 / m_scale);
        painter.setPen(pen);
        painter.setBrush(QColor(shape.fillColor.red(), shape.fillColor.green(), shape.fillColor.blue(), i == m_currentIndex ? 80 : 35));
        painter.drawRect(box);
        if (i == m_currentIndex) {
            painter.setBrush(Qt::white);
            const qreal handle = 4.0 / m_scale;
            for (int p = 0; p < shape.points.size(); ++p) {
                QRectF handleRect(shape.points[p].x() - handle, shape.points[p].y() - handle, handle * 2, handle * 2);
                painter.drawRect(handleRect);
            }
        }
        if (shape.paintLabel || m_paintLabels) {
            painter.setPen(Qt::yellow);
            painter.drawText(box.topLeft() + QPointF(2, -2), shape.label);
        }
    }
    painter.restore();
    emit frameRendered(frameTimer.nsecsElapsed() / 1000000.0);
}

void Canvas::mousePressEvent(QMouseEvent *event) {
    setFocus();
    m_lastMousePos = event->position().toPoint();
    m_startImagePos = clampToPixmap(imagePos(event->position()));
    if (event->button() == Qt::LeftButton) {
        if (m_editing) {
            int index = shapeAt(m_startImagePos);
            if (index >= 0) {
                selectIndex(index);
                m_hoverVertex = m_shapes[index].nearestVertex(m_startImagePos, 6.0 / m_scale);
                if (m_hoverVertex >= 0) {
                    m_movingVertex = true;
                } else {
                    m_moving = true;
                }
            }
        } else if (m_createMode) {
            m_creating = true;
            Shape shape = Shape::fromRect(QString(), QRectF(m_startImagePos, QSizeF(1, 1)), false);
            shape.lineColor = m_lineColor;
            shape.fillColor = m_fillColor;
            shape.paintLabel = m_paintLabels;
            m_shapes.push_back(shape);
            selectIndex(m_shapes.size() - 1);
        }
    } else if (event->button() == Qt::RightButton) {
        m_rightPanning = true;
        m_rightDragged = false;
        m_rightPressPos = event->position().toPoint();
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    QPoint current = event->position().toPoint();
    QPointF currentImagePos = clampToPixmap(imagePos(event->position()));

    if (m_rightPanning) {
        if (!m_rightDragged && (current - m_rightPressPos).manhattanLength() < QApplication::startDragDistance()) {
            return;
        }
        if (!m_rightDragged) {
            m_rightDragged = true;
            m_lastMousePos = current;
            return;
        }
        QPoint delta = current - m_lastMousePos;
        emit scrollRequested(delta.x(), delta.y());
        m_lastMousePos = current;
        return;
    }

    if (m_creating && hasSelection()) {
        QRectF rect(m_startImagePos, currentImagePos);
        if (m_drawSquare) {
            qreal side = qMin(qAbs(rect.width()), qAbs(rect.height()));
            rect.setWidth(rect.width() < 0 ? -side : side);
            rect.setHeight(rect.height() < 0 ? -side : side);
        }
        m_shapes[m_currentIndex] = Shape::fromRect(m_shapes[m_currentIndex].label, rect.normalized(), m_shapes[m_currentIndex].difficult);
        m_shapes[m_currentIndex].lineColor = m_lineColor;
        m_shapes[m_currentIndex].fillColor = m_fillColor;
        m_shapes[m_currentIndex].paintLabel = m_paintLabels;
        emit statusTextChanged(QString("Width: %1, Height: %2 / X: %3; Y: %4")
                                   .arg(qRound(qAbs(rect.width())))
                                   .arg(qRound(qAbs(rect.height())))
                                   .arg(qRound(currentImagePos.x()))
                                   .arg(qRound(currentImagePos.y())));
        emit shapesChanged();
        update();
    } else if (m_movingVertex && hasSelection() && m_hoverVertex >= 0) {
        QPointF delta = currentImagePos - imagePos(m_lastMousePos);
        Shape moved = m_shapes[m_currentIndex];
        QPointF target = clampToPixmap(moved.points[m_hoverVertex] + delta);
        delta = target - moved.points[m_hoverVertex];
        if (m_drawSquare) {
            const int oppositeIndex = (m_hoverVertex + 2) % 4;
            QPointF opposite = moved.points[oppositeIndex];
            qreal side = qMin(qAbs(target.x() - opposite.x()), qAbs(target.y() - opposite.y()));
            target.setX(opposite.x() + (target.x() < opposite.x() ? -side : side));
            target.setY(opposite.y() + (target.y() < opposite.y() ? -side : side));
            delta = clampToPixmap(target) - moved.points[m_hoverVertex];
        }
        m_shapes[m_currentIndex].moveVertexBy(m_hoverVertex, delta);
        m_lastMousePos = current;
        QRectF box = m_shapes[m_currentIndex].boundingRect();
        emit statusTextChanged(QString("Width: %1, Height: %2 / X: %3; Y: %4")
                                   .arg(qRound(box.width()))
                                   .arg(qRound(box.height()))
                                   .arg(qRound(currentImagePos.x()))
                                   .arg(qRound(currentImagePos.y())));
        emit shapesChanged();
        update();
    } else if (m_moving && hasSelection()) {
        QPointF delta = currentImagePos - imagePos(m_lastMousePos);
        delta = boundedDeltaForMove(m_shapes[m_currentIndex], delta);
        m_shapes[m_currentIndex].moveBy(delta);
        m_lastMousePos = current;
        QRectF box = m_shapes[m_currentIndex].boundingRect();
        emit statusTextChanged(QString("Width: %1, Height: %2 / X: %3; Y: %4")
                                   .arg(qRound(box.width()))
                                   .arg(qRound(box.height()))
                                   .arg(qRound(currentImagePos.x()))
                                   .arg(qRound(currentImagePos.y())));
        emit shapesChanged();
        update();
    } else {
        if (m_createMode && !m_editing) {
            setCursor(Qt::CrossCursor);
            emit statusTextChanged(QString("X: %1; Y: %2").arg(qRound(currentImagePos.x())).arg(qRound(currentImagePos.y())));
            return;
        }
        m_hoverShape = shapeAt(currentImagePos);
        m_hoverVertex = -1;
        if (m_hoverShape >= 0) {
            m_hoverVertex = m_shapes[m_hoverShape].nearestVertex(currentImagePos, 6.0 / m_scale);
            if (m_hoverVertex >= 0) {
                setCursor(Qt::CrossCursor);
                setToolTip("Click and drag to move point");
            } else {
                setCursor(Qt::OpenHandCursor);
                QRectF box = m_shapes[m_hoverShape].boundingRect();
                emit statusTextChanged(QString("Width: %1, Height: %2 / X: %3; Y: %4")
                                           .arg(qRound(box.width()))
                                           .arg(qRound(box.height()))
                                           .arg(qRound(currentImagePos.x()))
                                           .arg(qRound(currentImagePos.y())));
            }
        } else {
            unsetCursor();
            emit statusTextChanged(QString("X: %1; Y: %2").arg(qRound(currentImagePos.x())).arg(qRound(currentImagePos.y())));
        }
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    bool changed = m_creating || m_moving || m_movingVertex;
    if (m_creating && hasSelection() && isTooSmallToKeep(m_shapes[m_currentIndex].boundingRect())) {
        m_shapes.removeAt(m_currentIndex);
        m_currentIndex = -1;
        changed = true;
    }
    if (event->button() == Qt::RightButton && m_rightPanning && !m_rightDragged) {
        emit contextMenuRequested(event->globalPosition().toPoint(), clampToPixmap(imagePos(event->position())));
    }
    m_creating = false;
    m_moving = false;
    m_movingVertex = false;
    m_rightPanning = false;
    if (changed) {
        emit shapesChanged();
    }
    emit selectionChanged(m_currentIndex);
    if (m_createMode && !m_editing) {
        setCursor(Qt::CrossCursor);
    }
    update();
}

void Canvas::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
        setBrightness(m_brightness + event->angleDelta().y() / 120 * 5);
    } else if (event->angleDelta().y() != 0) {
        const double oldScale = m_scale;
        setScale(m_scale * (event->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15)));
        emit scaleChanged(oldScale, m_scale, event->position().toPoint());
    } else {
        emit scrollRequested(event->angleDelta().x(), 0);
    }
}

void Canvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        cancelDrawing();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_creating) {
            m_creating = false;
            emit shapesChanged();
            emit selectionChanged(m_currentIndex);
            update();
        }
        return;
    }
    if (!hasSelection()) {
        return;
    }
    QPointF delta;
    if (event->key() == Qt::Key_Left) delta = QPointF(-1, 0);
    if (event->key() == Qt::Key_Right) delta = QPointF(1, 0);
    if (event->key() == Qt::Key_Up) delta = QPointF(0, -1);
    if (event->key() == Qt::Key_Down) delta = QPointF(0, 1);
    if (!delta.isNull()) {
        delta = boundedDeltaForMove(m_shapes[m_currentIndex], delta);
        m_shapes[m_currentIndex].moveBy(delta);
        emit shapesChanged();
        update();
    }
}

QPointF Canvas::boundedTopLeftForCenter(const Shape &shape, const QPointF &center) const {
    QRectF box = shape.boundingRect();
    QPointF topLeft(center.x() - box.width() / 2.0, center.y() - box.height() / 2.0);
    if (!m_pixmap.isNull()) {
        topLeft.setX(qBound(0.0, topLeft.x(), static_cast<double>(m_pixmap.width()) - box.width()));
        topLeft.setY(qBound(0.0, topLeft.y(), static_cast<double>(m_pixmap.height()) - box.height()));
    }
    return topLeft;
}

QPointF Canvas::boundedDeltaForMove(const Shape &shape, const QPointF &delta) const {
    if (m_pixmap.isNull()) {
        return delta;
    }
    QRectF moved = shape.boundingRect().translated(delta);
    QPointF bounded = delta;
    if (moved.left() < 0) bounded.rx() -= moved.left();
    if (moved.top() < 0) bounded.ry() -= moved.top();
    if (moved.right() > m_pixmap.width()) bounded.rx() -= moved.right() - m_pixmap.width();
    if (moved.bottom() > m_pixmap.height()) bounded.ry() -= moved.bottom() - m_pixmap.height();
    return bounded;
}

QPointF Canvas::clampToPixmap(const QPointF &point) const {
    if (m_pixmap.isNull()) {
        return point;
    }
    return QPointF(qBound(0.0, point.x(), static_cast<double>(m_pixmap.width())),
                   qBound(0.0, point.y(), static_cast<double>(m_pixmap.height())));
}

QPointF Canvas::imagePos(const QPointF &widgetPos) const {
    return widgetPos / m_scale;
}

QRectF Canvas::widgetRect(const Shape &shape) const {
    QRectF rect = shape.boundingRect();
    return QRectF(rect.topLeft() * m_scale, QSizeF(rect.width() * m_scale, rect.height() * m_scale));
}

int Canvas::shapeAt(const QPointF &imagePoint) const {
    for (int i = m_shapes.size() - 1; i >= 0; --i) {
        if (m_shapes[i].contains(imagePoint)) {
            return i;
        }
    }
    return -1;
}

void Canvas::selectIndex(int index) {
    if (m_currentIndex >= 0 && m_currentIndex < m_shapes.size()) {
        m_shapes[m_currentIndex].selected = false;
    }
    m_currentIndex = (index >= 0 && index < m_shapes.size()) ? index : -1;
    if (hasSelection()) {
        m_shapes[m_currentIndex].selected = true;
    }
    emit selectionChanged(m_currentIndex);
    update();
}
