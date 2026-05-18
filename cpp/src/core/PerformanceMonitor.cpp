#include "core/PerformanceMonitor.h"

#include <QThread>

#ifdef _WIN32
#include <psapi.h>
#else
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
ULONGLONG fileTimeToUInt64(const FILETIME &fileTime) {
    return (static_cast<ULONGLONG>(fileTime.dwHighDateTime) << 32)
           | static_cast<ULONGLONG>(fileTime.dwLowDateTime);
}
#endif
}

PerformanceMonitor::PerformanceMonitor(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this)) {
#ifdef _WIN32
    m_processHandle = GetCurrentProcess();
#endif
    connect(m_timer, &QTimer::timeout, this, &PerformanceMonitor::updatePerformance);
}

void PerformanceMonitor::start(int intervalMs) {
    processCpuPercent();
    m_timer->start(intervalMs);
    updatePerformance();
}

void PerformanceMonitor::stop() {
    m_timer->stop();
}

QString PerformanceMonitor::getPerformanceText() const {
    return QStringLiteral("CPU %1% | MEM %2 MB")
        .arg(processCpuPercent(), 0, 'f', 1)
        .arg(processMemoryMb(), 0, 'f', 0);
}

void PerformanceMonitor::updatePerformance() {
    emit performanceUpdated(getPerformanceText());
}

double PerformanceMonitor::processMemoryMb() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX processMemory;
    if (GetProcessMemoryInfo(m_processHandle,
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&processMemory),
                             sizeof(processMemory))) {
        return static_cast<double>(processMemory.WorkingSetSize) / 1024.0 / 1024.0;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return static_cast<double>(usage.ru_maxrss) / 1024.0;
    }
#endif
    return 0.0;
}

double PerformanceMonitor::processCpuPercent() const {
#ifdef _WIN32
    FILETIME creationTime;
    FILETIME exitTime;
    FILETIME kernelTime;
    FILETIME userTime;
    if (!GetProcessTimes(m_processHandle, &creationTime, &exitTime, &kernelTime, &userTime)) {
        return 0.0;
    }

    FILETIME wallTime;
    GetSystemTimeAsFileTime(&wallTime);
    const ULONGLONG processTime = fileTimeToUInt64(kernelTime) + fileTimeToUInt64(userTime);
    const ULONGLONG currentWallTime = fileTimeToUInt64(wallTime);
    const ULONGLONG previousProcessTime = m_lastProcessTime;
    const ULONGLONG previousWallTime = m_lastWallTime;

    m_lastProcessTime = processTime;
    m_lastWallTime = currentWallTime;

    if (previousWallTime == 0 || currentWallTime <= previousWallTime) {
        return 0.0;
    }

    const int processorCount = qMax(1, QThread::idealThreadCount());
    const double usage = static_cast<double>(processTime - previousProcessTime)
                         / (static_cast<double>(currentWallTime - previousWallTime) * processorCount)
                         * 100.0;
    return qBound(0.0, usage, 100.0);
#else
    struct tms processTimes;
    const clock_t currentWallTime = times(&processTimes);
    if (currentWallTime == static_cast<clock_t>(-1)) {
        return 0.0;
    }

    const quint64 processTime = static_cast<quint64>(processTimes.tms_utime + processTimes.tms_stime);
    const quint64 previousProcessTime = m_lastProcessTime;
    const quint64 previousWallTime = m_lastWallTime;

    m_lastProcessTime = processTime;
    m_lastWallTime = static_cast<quint64>(currentWallTime);

    if (previousWallTime == 0 || static_cast<quint64>(currentWallTime) <= previousWallTime) {
        return 0.0;
    }

    const int processorCount = qMax(1, QThread::idealThreadCount());
    const double usage = static_cast<double>(processTime - previousProcessTime)
                         / (static_cast<double>(static_cast<quint64>(currentWallTime) - previousWallTime)
                            * processorCount)
                         * 100.0;
    return qBound(0.0, usage, 100.0);
#endif
}
