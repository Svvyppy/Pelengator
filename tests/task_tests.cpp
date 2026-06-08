#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "async/ManualResetEvent.h"
#include "async/Scheduler.h"
#include "async/Task.h"

namespace hydrv::tests {

hydrv::async::Task<> SetValue(int& value)
{
    value = 42;
    co_return;
}

hydrv::async::Task<int> ReturnValue()
{
    co_return 42;
}

hydrv::async::Task<> AwaitValue(int& value)
{
    value = co_await hydrv::tests::ReturnValue();
}

hydrv::async::Task<> WaitForEvent(hydrv::async::ManualResetEvent& event, int& progress)
{
    progress = 1;
    co_await event;
    progress = 2;
}

hydrv::async::Task<> RecordValue(std::vector<int>& values, int value)
{
    values.push_back(value);
    co_return;
}

bool criticalSectionActive = false;
int criticalSectionEntries = 0;
int criticalSectionExits   = 0;

uint32_t EnterCriticalSection() noexcept
{
    criticalSectionActive = true;
    ++criticalSectionEntries;
    return 1U;
}

void ExitCriticalSection(uint32_t interruptState) noexcept
{
    REQUIRE(interruptState == 1U);
    criticalSectionActive = false;
    ++criticalSectionExits;
}

hydrv::async::Task<> RecordCriticalSectionState(bool& resumedInsideCriticalSection)
{
    resumedInsideCriticalSection = criticalSectionActive;
    co_return;
}

} // namespace hydrv::tests

TEST_CASE("Task is lazy and runs when started")
{
    int value = 0;
    auto task = hydrv::tests::SetValue(value);

    REQUIRE(value == 0);
    REQUIRE_FALSE(task.done());

    task.start();

    REQUIRE(value == 42);
    REQUIRE(task.done());
}

TEST_CASE("Task transfers a result to an awaiting task")
{
    int value = 0;
    auto task = hydrv::tests::AwaitValue(value);

    task.start();

    REQUIRE(value == 42);
    REQUIRE(task.done());
}

TEST_CASE("Task move transfers coroutine ownership")
{
    int value        = 0;
    auto source      = hydrv::tests::SetValue(value);
    auto destination = std::move(source);

    REQUIRE(source.done());

    destination.start();

    REQUIRE(value == 42);
    REQUIRE(destination.done());
}

TEST_CASE("Event suspends task until Set")
{
    int progress = 0;
    hydrv::async::Scheduler scheduler;
    hydrv::async::ManualResetEvent event{ scheduler };
    auto task = hydrv::tests::WaitForEvent(event, progress);
    REQUIRE(progress == 0);
    task.start();
    REQUIRE(progress == 1);
    REQUIRE(task.done() == false);
    REQUIRE(event.hasWaiter() == true);

    event.set();

    REQUIRE(progress == 1);
    scheduler.runReady();

    REQUIRE(progress == 2);
    REQUIRE(task.done() == true);
    REQUIRE(event.hasWaiter() == false);
    REQUIRE(event.isSet() == true);
}

TEST_CASE("Already set event does not suspend task")
{
    int progress = 0;
    hydrv::async::Scheduler scheduler;
    hydrv::async::ManualResetEvent event{ scheduler };
    event.set();
    auto task = hydrv::tests::WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 2);
    REQUIRE(task.done() == true);
    REQUIRE(event.hasWaiter() == false);
}

TEST_CASE("Reset makes event awaitable again")
{
    int progress = 0;
    hydrv::async::Scheduler scheduler;
    hydrv::async::ManualResetEvent event{ scheduler };

    REQUIRE(event.set() == false);
    REQUIRE(event.isSet());

    event.reset();

    auto task = hydrv::tests::WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 1);
    REQUIRE_FALSE(task.done());
    REQUIRE(event.hasWaiter());

    REQUIRE(event.set());
    REQUIRE(progress == 1);
    scheduler.runReady();
    REQUIRE(progress == 2);
    REQUIRE(task.done());
}

TEST_CASE("Set remembers notification without waiter")
{
    int progress = 0;
    hydrv::async::Scheduler scheduler;
    hydrv::async::ManualResetEvent event{ scheduler };

    REQUIRE(event.set() == false);
    REQUIRE(event.isSet());
    REQUIRE_FALSE(event.hasWaiter());

    auto task = hydrv::tests::WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 2);
    REQUIRE(task.done());
}

TEST_CASE("Repeated Set does not resume a completed task")
{
    int progress = 0;
    hydrv::async::Scheduler scheduler;
    hydrv::async::ManualResetEvent event{ scheduler };
    auto task = hydrv::tests::WaitForEvent(event, progress);
    task.start();

    REQUIRE(event.set());
    REQUIRE(progress == 1);
    scheduler.runReady();
    REQUIRE(progress == 2);
    REQUIRE(task.done());

    REQUIRE(event.set() == false);
    REQUIRE(progress == 2);
    REQUIRE(event.isSet());
    REQUIRE_FALSE(event.hasWaiter());
}

TEST_CASE("Simple task coroutine")
{
    int value = 0;
    auto task = hydrv::tests::SetValue(value);
    hydrv::async::Scheduler scheduler;
    REQUIRE(scheduler.schedule(task.handle()));
    REQUIRE(value == 0);

    scheduler.runOne();

    REQUIRE(value == 42);
    REQUIRE(task.done());
}
TEST_CASE("Scheduler runs tasks in FIFO order")
{
    hydrv::async::Scheduler scheduler;
    std::vector<int> values;
    std::vector<hydrv::async::Task<>> tasks;
    tasks.reserve(3);

    for (int value = 1; value <= 3; ++value) {
        tasks.push_back(hydrv::tests::RecordValue(values, value));
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
    hydrv::async::Scheduler scheduler;
    std::vector<int> values;
    std::vector<hydrv::async::Task<>> tasks;
    tasks.reserve(hydrv::async::Scheduler::kCapacity + 1);

    for (std::size_t index = 0; index < hydrv::async::Scheduler::kCapacity + 1; ++index) {
        tasks.push_back(hydrv::tests::RecordValue(values, static_cast<int>(index)));
        const bool scheduled = scheduler.schedule(tasks.back().handle());
        REQUIRE(scheduled == (index < hydrv::async::Scheduler::kCapacity));
    }

    REQUIRE(scheduler.size() == hydrv::async::Scheduler::kCapacity);
    REQUIRE(scheduler.overflowCount() == 1);

    scheduler.runReady();
    REQUIRE(values.size() == hydrv::async::Scheduler::kCapacity);
}

TEST_CASE("Scheduler preserves FIFO order across queue wrap-around")
{
    hydrv::async::Scheduler scheduler;
    std::vector<int> values;
    std::vector<hydrv::async::Task<>> tasks;
    tasks.reserve(10);

    for (int value = 0; value < 6; ++value) {
        tasks.push_back(hydrv::tests::RecordValue(values, value));
        REQUIRE(scheduler.schedule(tasks.back().handle()));
    }

    REQUIRE(scheduler.runOne());
    REQUIRE(scheduler.runOne());
    REQUIRE(scheduler.runOne());

    for (int value = 6; value < 10; ++value) {
        tasks.push_back(hydrv::tests::RecordValue(values, value));
        REQUIRE(scheduler.schedule(tasks.back().handle()));
    }

    scheduler.runReady();

    REQUIRE(values == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    REQUIRE(scheduler.isEmpty());
}

TEST_CASE("Scheduler Coroutine")
{
    int progress = 0;
    hydrv::async::Scheduler scheduler;
    hydrv::async::ManualResetEvent event{ scheduler };

    auto task = hydrv::tests::WaitForEvent(event, progress);
    task.start();

    REQUIRE(progress == 1);

    event.set();

    REQUIRE(progress == 1);
    REQUIRE_FALSE(task.done());
    REQUIRE(scheduler.size() == 1);

    scheduler.runReady();

    REQUIRE(progress == 2);
    REQUIRE(task.done());
}
TEST_CASE("Scheduler resumes coroutine after leaving critical section")
{
    hydrv::tests::criticalSectionActive  = false;
    hydrv::tests::criticalSectionEntries = 0;
    hydrv::tests::criticalSectionExits   = 0;

    hydrv::async::Scheduler scheduler{ hydrv::tests::EnterCriticalSection,
                                      hydrv::tests::ExitCriticalSection };
    bool resumedInsideCriticalSection = true;
    auto task = hydrv::tests::RecordCriticalSectionState(resumedInsideCriticalSection);

    REQUIRE(scheduler.schedule(task.handle()));
    REQUIRE(scheduler.runOne());

    REQUIRE_FALSE(resumedInsideCriticalSection);
    REQUIRE(hydrv::tests::criticalSectionEntries == 2);
    REQUIRE(hydrv::tests::criticalSectionExits == 2);
}
