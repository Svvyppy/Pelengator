#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "async/ManualResetEvent.h"
#include "async/Scheduler.h"
#include "async/Task.h"

namespace {

async::Task<> SetValue(int& value)
{
    value = 42;
    co_return;
}

async::Task<int> ReturnValue()
{
    co_return 42;
}

async::Task<> AwaitValue(int& value)
{
    value = co_await ReturnValue();
}

async::Task<> WaitForEvent(async::ManualResetEvent& event, int& progress)
{
    progress = 1;
    co_await event;
    progress = 2;
}

async::Task<> RecordValue(std::vector<int>& values, int value)
{
    values.push_back(value);
    co_return;
}

} // namespace

TEST_CASE("Task is lazy and runs when started")
{
    int value = 0;
    auto task = SetValue(value);

    REQUIRE(value == 0);
    REQUIRE_FALSE(task.Done());

    task.start();

    REQUIRE(value == 42);
    REQUIRE(task.Done());
}

TEST_CASE("Task transfers a result to an awaiting task")
{
    int value = 0;
    auto task = AwaitValue(value);

    task.start();

    REQUIRE(value == 42);
    REQUIRE(task.Done());
}

TEST_CASE("Task move transfers coroutine ownership")
{
    int value        = 0;
    auto source      = SetValue(value);
    auto destination = std::move(source);

    REQUIRE(source.Done());

    destination.start();

    REQUIRE(value == 42);
    REQUIRE(destination.Done());
}

TEST_CASE("Event suspends task until Set")
{
    int progress = 0;
    async::Scheduler scheduler;
    async::ManualResetEvent event{ scheduler };
    auto task = WaitForEvent(event, progress);
    REQUIRE(progress == 0);
    task.start();
    REQUIRE(progress == 1);
    REQUIRE(task.Done() == false);
    REQUIRE(event.HasWaiter() == true);

    event.Set();

    REQUIRE(progress == 1);
    scheduler.runReady();

    REQUIRE(progress == 2);
    REQUIRE(task.Done() == true);
    REQUIRE(event.HasWaiter() == false);
    REQUIRE(event.IsSet() == true);
}

TEST_CASE("Already set event does not suspend task")
{
    int progress = 0;
    async::Scheduler scheduler;
    async::ManualResetEvent event{ scheduler };
    event.Set();
    auto task = WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 2);
    REQUIRE(task.Done() == true);
    REQUIRE(event.HasWaiter() == false);
}

TEST_CASE("Reset makes event awaitable again")
{
    int progress = 0;
    async::Scheduler scheduler;
    async::ManualResetEvent event{ scheduler };

    REQUIRE(event.Set() == false);
    REQUIRE(event.IsSet());

    event.Reset();

    auto task = WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 1);
    REQUIRE_FALSE(task.Done());
    REQUIRE(event.HasWaiter());

    REQUIRE(event.Set());
    REQUIRE(progress == 1);
    scheduler.runReady();
    REQUIRE(progress == 2);
    REQUIRE(task.Done());
}

TEST_CASE("Set remembers notification without waiter")
{
    int progress = 0;
    async::Scheduler scheduler;
    async::ManualResetEvent event{ scheduler };

    REQUIRE(event.Set() == false);
    REQUIRE(event.IsSet());
    REQUIRE_FALSE(event.HasWaiter());

    auto task = WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 2);
    REQUIRE(task.Done());
}

TEST_CASE("Repeated Set does not resume a completed task")
{
    int progress = 0;
    async::Scheduler scheduler;
    async::ManualResetEvent event{ scheduler };
    auto task = WaitForEvent(event, progress);
    task.start();

    REQUIRE(event.Set());
    REQUIRE(progress == 1);
    scheduler.runReady();
    REQUIRE(progress == 2);
    REQUIRE(task.Done());

    REQUIRE(event.Set() == false);
    REQUIRE(progress == 2);
    REQUIRE(event.IsSet());
    REQUIRE_FALSE(event.HasWaiter());
}

TEST_CASE("Simple task coroutine")
{
    int value = 0;
    auto task = SetValue(value);
    async::Scheduler scheduler;
    REQUIRE(scheduler.schedule(task.handle()));
    REQUIRE(value == 0);

    scheduler.runOne();

    REQUIRE(value == 42);
    REQUIRE(task.Done());
}
TEST_CASE("Scheduler runs tasks in FIFO order")
{
    async::Scheduler scheduler;
    std::vector<int> values;
    std::vector<async::Task<>> tasks;
    tasks.reserve(3);

    for (int value = 1; value <= 3; ++value) {
        tasks.push_back(RecordValue(values, value));
        REQUIRE(scheduler.schedule(tasks.back().handle()));
    }

    REQUIRE(values.empty());
    REQUIRE(scheduler.size() == 3);

    scheduler.runReady();

    REQUIRE(values == std::vector<int>{ 1, 2, 3 });
    REQUIRE(scheduler.isEmpty());
}

TEST_CASE("Scheduler reports overflow at full capacity")
{
    async::Scheduler scheduler;
    std::vector<int> values;
    std::vector<async::Task<>> tasks;
    tasks.reserve(async::Scheduler::Capacity + 1);

    for (std::size_t index = 0; index < async::Scheduler::Capacity + 1; ++index) {
        tasks.push_back(RecordValue(values, static_cast<int>(index)));
        const bool scheduled = scheduler.schedule(tasks.back().handle());
        REQUIRE(scheduled == (index < async::Scheduler::Capacity));
    }

    REQUIRE(scheduler.size() == async::Scheduler::Capacity);
    REQUIRE(scheduler.overflowCount() == 1);

    scheduler.runReady();
    REQUIRE(values.size() == async::Scheduler::Capacity);
}

TEST_CASE("Scheduler preserves FIFO order across queue wrap-around")
{
    async::Scheduler scheduler;
    std::vector<int> values;
    std::vector<async::Task<>> tasks;
    tasks.reserve(10);

    for (int value = 0; value < 6; ++value) {
        tasks.push_back(RecordValue(values, value));
        REQUIRE(scheduler.schedule(tasks.back().handle()));
    }

    REQUIRE(scheduler.runOne());
    REQUIRE(scheduler.runOne());
    REQUIRE(scheduler.runOne());

    for (int value = 6; value < 10; ++value) {
        tasks.push_back(RecordValue(values, value));
        REQUIRE(scheduler.schedule(tasks.back().handle()));
    }

    scheduler.runReady();

    REQUIRE(values == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    REQUIRE(scheduler.isEmpty());
}

TEST_CASE("Scheduler Coroutine")
{
    int progress = 0;
    async::Scheduler scheduler;
    async::ManualResetEvent event{ scheduler };

    auto task = WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 1);

    event.Set();

    REQUIRE(progress == 1);
    REQUIRE_FALSE(task.Done());
    REQUIRE(scheduler.size() == 1);

    scheduler.runReady();

    REQUIRE(progress == 2);
    REQUIRE(task.Done());
}