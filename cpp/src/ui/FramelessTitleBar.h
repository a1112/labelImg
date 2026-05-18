#pragma once

#include <QFrame>
#include <QPoint>

class QHBoxLayout;
class QLabel;
class QToolButton;

class FramelessTitleBar : public QFrame {
    Q_OBJECT

public:
    explicit FramelessTitleBar(QWidget *parent = nullptr);
    void setTitle(const QString &title);
    void setMaximized(bool maximized);
    void setToolWidget(QWidget *widget);

signals:
    void dragRequested();
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QToolButton *createButton(const QString &text, const QString &toolTip, const QString &objectName) const;

    QHBoxLayout *m_layout = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QWidget *m_toolWidget = nullptr;
    QToolButton *m_maximizeButton = nullptr;
    QPoint m_pressPosition;
    bool m_dragStarted = false;
};
