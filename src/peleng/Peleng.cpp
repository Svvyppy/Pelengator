#include "Peleng.hpp"

#include <array>
#include <cstdio>

#include "Hw.h"

namespace hydrv::peleng {

inline constexpr uint32_t kTelemetryPeriod = 16U;

Peleng::Peleng()
    : _scheduler{ hw::enterCriticalSection, hw::exitCriticalSection }
    , _recorder{ _scheduler }
{}

[[noreturn]] void Peleng::run()
{
    hw::initialize(_recorder, _telemetry);

    (void)_telemetry.write("Peleng Init\r\n");
    if (!_recorder.startRecording()) {
        hw::stop();
    }

    _signalProcessingTask = processSignals();
    _signalProcessingTask.start();
    (void)_telemetry.write("Signal acquisition started\r\n");

    while (true) {
        _telemetry.process();
        _scheduler.runReady();
    }
}

async::Task<> Peleng::processSignals()
{
    while (true) {
        const hw::SignalRecorder::RecordedBlock recordedBlock = co_await _recorder.nextBlock();
        processSignal(recordedBlock);

        ++_processedFrames;
        if ((_processedFrames % kTelemetryPeriod) == 0U) {
            publishMeasurements(estimateDelays(_filteredSignals));
        }
    }
}

void Peleng::processSignal(const hw::SignalRecorder::RecordedBlock& recordedBlock)
{
    for (std::size_t channel = 0U; channel < kAdcChannelCount; ++channel) {
        convertSignal(recordedBlock.channelSamples[channel].data(), _squaredSignals[channel].data());
        _filters[channel].apply(_squaredSignals[channel].data(), _filteredSignals[channel].data(), kSignalBlockSize);
    }
}

void Peleng::publishMeasurements(const DelayMeasurements& measurements)
{
    if (!measurements.valid || !measurements.directionValid) {
        return;
    }

    const int32_t bearingTenths       = roundToInteger(measurements.bearingDegrees * 10.0f);
    const int32_t elevationTenths     = roundToInteger(measurements.elevationDegrees * 10.0f);
    const uint32_t bearingMagnitude   = magnitude(bearingTenths);
    const uint32_t elevationMagnitude = magnitude(elevationTenths);

    std::array<char, 128U> line{};
    const int length = std::snprintf(line.data(),
                                     line.size(),
                                     "D12=%ldus D13=%ldus D14=%ldus P=%s%lu.%ludeg E=%s%lu.%ludeg\r\n",
                                     static_cast<long>(roundToInteger(measurements.channel2Microseconds)),
                                     static_cast<long>(roundToInteger(measurements.channel3Microseconds)),
                                     static_cast<long>(roundToInteger(measurements.channel4Microseconds)),
                                     (bearingTenths < 0) ? "-" : "",
                                     static_cast<unsigned long>(bearingMagnitude / 10U),
                                     static_cast<unsigned long>(bearingMagnitude % 10U),
                                     (elevationTenths < 0) ? "-" : "",
                                     static_cast<unsigned long>(elevationMagnitude / 10U),
                                     static_cast<unsigned long>(elevationMagnitude % 10U));

    if (length > 0 && static_cast<std::size_t>(length) < line.size()) {
        (void)_telemetry.write({ line.data(), static_cast<std::size_t>(length) });
    }
}

void Peleng::convertSignal(const uint16_t* source, q15_t* destination)
{
    constexpr uint32_t kAdcMask    = 0x0FFFU;
    constexpr int32_t kAdcMidpoint = 2048;
    constexpr int32_t kQ15Shift    = 4;
    constexpr int32_t kQ15Maximum  = 32767;

    for (std::size_t sample = 0U; sample < kSignalBlockSize; ++sample) {
        const int32_t centered = (static_cast<int32_t>(source[sample] & kAdcMask) - kAdcMidpoint) << kQ15Shift;
        const int32_t squared  = (centered * centered) >> 15;
        destination[sample]    = static_cast<q15_t>((squared > kQ15Maximum) ? kQ15Maximum : squared);
    }
}

int32_t Peleng::roundToInteger(float value)
{
    return static_cast<int32_t>((value >= 0.0f) ? value + 0.5f : value - 0.5f);
}

uint32_t Peleng::magnitude(int32_t value)
{
    return (value >= 0) ? static_cast<uint32_t>(value) : static_cast<uint32_t>(-(value + 1)) + 1U;
}

} // namespace hydrv::peleng
