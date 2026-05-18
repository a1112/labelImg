#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#ifdef _WIN32
#include <windows.h>
#endif

class PerformanceMonitor : public QObject {
    Q_OBJECT

public:
    explicit PerformanceMonitor(QObject *parent = nullptr);

    void start(int intervalMs = 1000);
    void stop();
    QString getPerformanceText() const;

signals:
    void performanceUpdated(const QString &text);

private:
    double processMemoryMb() const;
    double processCpuPercent() const;
    void updatePerformance();

    QTimer *m_timer = nullptr;

#ifdef _WIN32
    HANDLE m_processHandle = nullptr;
    mutable ULONGLONG m_lastProcessTime = 0;
    mutable ULONGLONG m_lastWallTime = 0;
#else
    mutable quint64 m_lastProcessTime = 0;
    mutable quint64 m_lastWallTime = 0;
#endif
};
