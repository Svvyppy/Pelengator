#pragma once

#include <cassert>
#include <coroutine>
#include <cstddef>

#include "Scheduler.h"

namespace async {

class ManualResetEvent
{
public:
    explicit ManualResetEvent(Scheduler& scheduler)
        : _scheduler{ scheduler }
    {}
    bool await_ready() const noexcept
    {
        return IsSet();
    }
    bool await_suspend(std::coroutine_handle<> awaiting) noexcept
    {
        if (IsSet()) {
            return false;
        }
        assert(!_waiter);
        _waiter = awaiting;
        return true;
    }
    void await_resume() const noexcept
    {}

    bool Set() noexcept
    {
        _is_set = true;
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
    void Reset() noexcept
    {
        _is_set = false;
    }

    bool IsSet() const noexcept
    {
        return _is_set;
    }
    bool HasWaiter() const noexcept
    {
        return static_cast<bool>(_waiter);
    }

private:
    Scheduler& _scheduler;
    bool _is_set                    = false;
    std::coroutine_handle<> _waiter = nullptr;
};

} // namespace async