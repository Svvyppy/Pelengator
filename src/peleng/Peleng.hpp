#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "DelayEstimator.hpp"
#include "Filter.hpp"
#include "Scheduler.h"
#include "SignalRecorder.h"
#include "Task.h"
#include "UartTelemetry.h"

namespace hydrv::peleng {

class Peleng
{
public:
    Peleng();
    [[noreturn]] void run();

private:
    async::Task<> processSignals();
    void processSignal(const hw::SignalRecorder::RecordedBlock& recordedBlock);
    void publishMeasurements(const DelayMeasurements& measurements);
    static void convertSignal(const uint16_t* source, q15_t* destination);
    static int32_t roundToInteger(float value);
    static uint32_t magnitude(int32_t value);

    async::Scheduler _scheduler;
    hw::SignalRecorder _recorder;
    hw::UartTelemetry _telemetry{};
    async::Task<> _signalProcessingTask{};
    ChannelSignalBlocks _squaredSignals{};
    ChannelSignalBlocks _filteredSignals{};
    std::array<Filter, kAdcChannelCount> _filters{};
    uint32_t _processedFrames = 0U;
};

} // namespace hydrv::peleng
