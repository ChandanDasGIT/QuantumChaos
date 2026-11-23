#pragma once

#include <cstdint>
#include <functional>

// What the backend needs from the host to wait: the clock, and "run this later" on the same thread.
namespace platform {

class Scheduler {
public:
    virtual ~Scheduler() = default;
    // Milliseconds since 1970-01-01 UTC.
    virtual std::int64_t nowMs() const = 0;
    // Runs `fn` once, at least `delayMs` from now (0: as soon as the event loop is free).
    virtual void after(std::int64_t delayMs, std::function<void()> fn) = 0;
    // Runs `fn` on the scheduler's own thread soon. Unlike after(), safe to call from any thread
    // (a worker hands its results back this way).
    virtual void post(std::function<void()> fn) = 0;
};

} // namespace platform
