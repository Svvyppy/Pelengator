#pragma once

#include <array>
#include <coroutine>
#include <cstddef>

namespace async {

class Scheduler
{
public:
    static constexpr std::size_t Capacity{ 8 };

    bool schedule(std::coroutine_handle<> handle) noexcept
    {
        if (!handle || handle.done()) {
            return false;
        }
        if (_count >= Capacity) {
            ++_overflowCount;
            return false;
        }

        const std::size_t tail = (_head + _count) % Capacity;
        _queue[tail]           = handle;
        ++_count;
        return true;
    }

    bool runOne() noexcept
    {
        if (isEmpty()) {
            return false;
        }

        const std::coroutine_handle<> handle = _queue[_head];

        _queue[_head] = nullptr;
        _head         = (_head + 1) % Capacity;
        --_count;

        handle.resume();
        return true;
    }

    void runReady() noexcept
    {
        while (runOne()) {
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept
    {
        return _count == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return _count;
    }

    [[nodiscard]] std::size_t overflowCount() const noexcept
    {
        return _overflowCount;
    }

private:
    std::array<std::coroutine_handle<>, Capacity> _queue{};
    std::size_t _head          = 0;
    std::size_t _count         = 0;
    std::size_t _overflowCount = 0;
};

} // namespace async
