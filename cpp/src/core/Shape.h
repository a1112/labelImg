#pragma once

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

class Shape {
public:
    QString label;
    QVector<QPointF> points;
    QColor lineColor = QColor(0, 255, 0, 128);
    QColor fillColor = QColor(255, 0, 0, 128);
    bool selected = false;
    bool visible = true;
    bool difficult = false;
    bool paintLabel = false;

    static Shape fromRect(const QString &label, const QRectF &rect, bool difficult);

    QRectF boundingRect() const;
    bool contains(const QPointF &point) const;
    int nearestVertex(const QPointF &point, qreal epsilon) const;
    void moveBy(const QPointF &delta);
    void moveVertexBy(int index, const QPointF &delta);
    Shape copy() const;
    void setVisible(bool value);
};
