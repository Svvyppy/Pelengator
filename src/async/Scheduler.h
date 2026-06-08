#pragma once

#include <array>
#include <coroutine>
#include <cstddef>
#include <cstdint>

namespace hydrv::async {

class Scheduler
{
public:
    using EnterCriticalSection = uint32_t (*)() noexcept;
    using ExitCriticalSection  = void (*)(uint32_t) noexcept;

    static constexpr std::size_t kCapacity = 8U;

    Scheduler(EnterCriticalSection enterCriticalSection = noCriticalSection,
              ExitCriticalSection exitCriticalSection   = noCriticalSection) noexcept
        : _enterCriticalSection{ enterCriticalSection }
        , _exitCriticalSection{ exitCriticalSection }
    {}

    bool schedule(std::coroutine_handle<> handle) noexcept
    {
        if (!handle || handle.done()) {
            return false;
        }

        const uint32_t interruptState = _enterCriticalSection();
        if (_count == kCapacity) {
            ++_overflowCount;
            _exitCriticalSection(interruptState);
            return false;
        }

        _queue[(_head + _count) % kCapacity] = handle;
        ++_count;
        _exitCriticalSection(interruptState);
        return true;
    }

    bool runOne() noexcept
    {
        const uint32_t interruptState = _enterCriticalSection();
        if (_count == 0U) {
            _exitCriticalSection(interruptState);
            return false;
        }

        const std::coroutine_handle<> handle = _queue[_head];
        _queue[_head]                        = nullptr;
        _head                                = (_head + 1U) % kCapacity;
        --_count;
        _exitCriticalSection(interruptState);

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
        return _count == 0U;
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
    static uint32_t noCriticalSection() noexcept
    {
        return 0U;
    }
    static void noCriticalSection(uint32_t) noexcept
    {}

    EnterCriticalSection _enterCriticalSection;
    ExitCriticalSection _exitCriticalSection;
    std::array<std::coroutine_handle<>, kCapacity> _queue{};
    std::size_t _head          = 0U;
    std::size_t _count         = 0U;
    std::size_t _overflowCount = 0U;
};

} // namespace hydrv::async
