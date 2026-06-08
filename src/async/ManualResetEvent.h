#pragma once

#include <cassert>
#include <coroutine>
#include <cstddef>

#include "Scheduler.h"

namespace hydrv::async {

class ManualResetEvent
{
public:
    explicit ManualResetEvent(Scheduler& scheduler)
        : _scheduler{ scheduler }
    {}
    bool await_ready() const noexcept
    {
        return isSet();
    }
    bool await_suspend(std::coroutine_handle<> awaiting) noexcept
    {
        if (isSet()) {
            return false;
        }
        assert(!_waiter);
        _waiter = awaiting;
        return true;
    }
    void await_resume() const noexcept
    {}

    bool set() noexcept
    {
        _isSet = true;
        if (!_waiter) {
            return false;
        }

        const std::coroutine_handle<> waiter = _waiter;
        if (!_scheduler.schedule(waiter)) {
            return false;
        }

        _waiter = nullptr;
        return true;
    }
    void reset() noexcept
    {
        _isSet = false;
    }

    bool isSet() const noexcept
    {
        return _isSet;
    }
    bool hasWaiter() const noexcept
    {
        return static_cast<bool>(_waiter);
    }

private:
    Scheduler& _scheduler;
    bool _isSet                     = false;
    std::coroutine_handle<> _waiter = nullptr;
};

} // namespace hydrv::async
