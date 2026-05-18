#include "core/Shape.h"

#include <algorithm>
#include <cmath>

Shape Shape::fromRect(const QString &label, const QRectF &rect, bool difficult) {
    Shape shape;
    shape.label = label;
    shape.difficult = difficult;
    shape.points = {
        rect.topLeft(),
        rect.topRight(),
        rect.bottomRight(),
        rect.bottomLeft(),
    };
    return shape;
}

QRectF Shape::boundingRect() const {
    if (points.isEmpty()) {
        return {};
    }

    qreal minX = points[0].x();
    qreal minY = points[0].y();
    qreal maxX = points[0].x();
    qreal maxY = points[0].y();
    for (const QPointF &point : points) {
        minX = std::min(minX, point.x());
        minY = std::min(minY, point.y());
        maxX = std::max(maxX, point.x());
        maxY = std::max(maxY, point.y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized();
}

bool Shape::contains(const QPointF &point) const {
    return visible && boundingRect().contains(point);
}

int Shape::nearestVertex(const QPointF &point, qreal epsilon) const {
    int nearest = -1;
    qreal bestDistance = epsilon;
    for (int i = 0; i < points.size(); ++i) {
        const QPointF delta = points[i] - point;
        const qreal distance = std::hypot(delta.x(), delta.y());
        if (distance <= bestDistance) {
            bestDistance = distance;
            nearest = i;
        }
    }
    return nearest;
}

void Shape::moveBy(const QPointF &delta) {
    for (QPointF &point : points) {
        point += delta;
    }
}

void Shape::moveVertexBy(int index, const QPointF &delta) {
    if (points.size() != 4 || index < 0 || index >= points.size()) {
        return;
    }

    points[index] += delta;
    const int leftIndex = (index + 1) % 4;
    const int rightIndex = (index + 3) % 4;
    if (index % 2 == 0) {
        points[leftIndex].ry() += delta.y();
        points[rightIndex].rx() += delta.x();
    } else {
        points[leftIndex].rx() += delta.x();
        points[rightIndex].ry() += delta.y();
    }
}

Shape Shape::copy() const {
    return *this;
}

void Shape::setVisible(bool value) {
    visible = value;
}
