#include "core/LabelListModel.h"

#include <stdexcept>

void LabelListModel::add(const Shape &shape) {
    m_shapes.push_back(shape);
    m_currentIndex = m_shapes.size() - 1;
}

void LabelListModel::clear() {
    m_shapes.clear();
    m_currentIndex = -1;
}

void LabelListModel::clearSelection() {
    m_currentIndex = -1;
}

void LabelListModel::selectAdjacent(int step) {
    if (m_shapes.isEmpty()) {
        m_currentIndex = -1;
        return;
    }
    if (m_currentIndex < 0 || m_currentIndex >= m_shapes.size()) {
        m_currentIndex = 0;
        return;
    }
    m_currentIndex = (m_currentIndex + step) % m_shapes.size();
    if (m_currentIndex < 0) {
        m_currentIndex += m_shapes.size();
    }
}

void LabelListModel::select(int index) {
    if (index < 0 || index >= m_shapes.size()) {
        m_currentIndex = -1;
        return;
    }
    m_currentIndex = index;
}

void LabelListModel::removeCurrent() {
    if (m_currentIndex < 0 || m_currentIndex >= m_shapes.size()) {
        return;
    }
    int removed = m_currentIndex;
    m_shapes.removeAt(m_currentIndex);
    if (m_shapes.isEmpty()) {
        m_currentIndex = -1;
    } else if (removed < m_shapes.size()) {
        m_currentIndex = removed;
    } else {
        m_currentIndex = 0;
    }
}

int LabelListModel::count() const {
    return m_shapes.size();
}

int LabelListModel::currentIndex() const {
    return m_currentIndex;
}

const Shape &LabelListModel::current() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_shapes.size()) {
        throw std::out_of_range("No current label");
    }
    return m_shapes[m_currentIndex];
}

QVector<Shape> &LabelListModel::shapes() {
    return m_shapes;
}

const QVector<Shape> &LabelListModel::shapes() const {
    return m_shapes;
}
