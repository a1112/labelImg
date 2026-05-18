#pragma once

#include "core/Shape.h"

#include <QVector>

class LabelListModel {
public:
    void add(const Shape &shape);
    void clear();
    void clearSelection();
    void selectAdjacent(int step);
    void select(int index);
    void removeCurrent();

    int count() const;
    int currentIndex() const;
    const Shape &current() const;
    QVector<Shape> &shapes();
    const QVector<Shape> &shapes() const;

private:
    QVector<Shape> m_shapes;
    int m_currentIndex = -1;
};
