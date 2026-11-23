#include "schedulerWrap.h"

#include <QDateTime>
#include <QTimer>

#include <algorithm>
#include <limits>

SchedulerWrap::SchedulerWrap(QObject* parent) : QObject(parent)
{
}

std::int64_t SchedulerWrap::nowMs() const
{
    return QDateTime::currentMSecsSinceEpoch();
}

void SchedulerWrap::post(std::function<void()> fn)
{
    QMetaObject::invokeMethod(this, std::move(fn), Qt::QueuedConnection);
}

void SchedulerWrap::after(std::int64_t delayMs, std::function<void()> fn)
{
    // A QTimer counts in int milliseconds (about 24 days); longer waits are not needed here.
    const auto ms = static_cast<int>(std::clamp<std::int64_t>(delayMs, 0, std::numeric_limits<int>::max()));
    QTimer::singleShot(ms, Qt::CoarseTimer, this, std::move(fn));
}
