#pragma once

#include "common/platform/scheduler.h"

#include <QObject>

// The backend's Scheduler on Qt's clock and timers. Pending jobs die with this object.
class SchedulerWrap : public QObject, public platform::Scheduler {
    Q_OBJECT

public:
    explicit SchedulerWrap(QObject* parent = nullptr);

    std::int64_t nowMs() const override;
    void after(std::int64_t delayMs, std::function<void()> fn) override;
    void post(std::function<void()> fn) override;
};
