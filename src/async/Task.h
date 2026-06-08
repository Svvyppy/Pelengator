#pragma once

#include <cassert>
#include <coroutine>
#include <optional>
#include <type_traits>
#include <utility>

namespace hydrv::async {

namespace detail {

template<typename T>
class TaskResult
{
public:
    void return_value(T value)
    {
        _result.emplace(std::move(value));
    }

    T takeResult()
    {
        assert(_result.has_value());
        return std::move(*_result);
    }

private:
    std::optional<T> _result;
};

template<>
class TaskResult<void>
{
public:
    void return_void() noexcept
    {}

    void takeResult() const noexcept
    {}
};

} // namespace detail

template<typename T = void>
class Task
{
public:
    Task() noexcept = default;

    struct promise_type : detail::TaskResult<T>
    {
        std::coroutine_handle<> continuation_ = nullptr;

        Task get_return_object()
        {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        std::suspend_always initial_suspend() const noexcept
        {
            return {};
        }

        auto final_suspend() noexcept
        {
            struct Awaiter
            {
                std::coroutine_handle<> continuation;

                bool await_ready() const noexcept
                {
                    return false;
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept
                {
                    return continuation ? continuation : std::noop_coroutine();
                }

                void await_resume() const noexcept
                {}
            };
            return Awaiter{ continuation_ };
        }

        [[noreturn]] void unhandled_exception() noexcept
        {
            assert(false);
            __builtin_trap();
        }
    };

    Task(Task&& other) noexcept
        : _handle(std::exchange(other._handle, nullptr))
    {}

    Task& operator=(Task&& other) noexcept
    {
        if (this != &other) {
            if (_handle) {
                _handle.destroy();
            }
            _handle = std::exchange(other._handle, nullptr);
        }
        return *this;
    }

    Task(const Task&)            = delete;
    Task& operator=(const Task&) = delete;

    ~Task()
    {
        if (_handle) {
            _handle.destroy();
        }
    }

    void start()
    {
        if (_handle && !_handle.done()) {
            _handle.resume();
        }
    }

    bool await_ready() const noexcept
    {
        return !_handle || _handle.done();
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) noexcept
    {
        assert(_handle);
        auto& promise         = _handle.promise();
        promise.continuation_ = awaiting;
        return _handle;
    }

    decltype(auto) await_resume()
    {
        assert(_handle);
        if constexpr (!std::is_void_v<T>) {
            return _handle.promise().takeResult();
        }
        else {
            _handle.promise().takeResult();
        }
    }

    bool done() const
    {
        return !_handle || _handle.done();
    }

    std::coroutine_handle<> handle() const noexcept
    {
        return _handle;
    }

private:
    explicit Task(std::coroutine_handle<promise_type> handle)
        : _handle(handle)
    {}

    std::coroutine_handle<promise_type> _handle = nullptr;
};

} // namespace hydrv::async
