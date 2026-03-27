#include "DelayEstimator.hpp"

namespace
{
constexpr float kMicrosecondsPerSecond = 1000000.0f;
}

namespace peleng
{
ThresholdCrossing FindThresholdCrossing(const HalfBuffer &buffer)
{
    for (std::size_t index = 0U; index < DMA_HALF_BUFFER_SIZE; ++index)
    {
        const float value = buffer[index];
        if (value > SIGNAL_THRESHOLD)
        {
            return ThresholdCrossing{index, value, true};
        }
    }

    return ThresholdCrossing{};
}

DelayMeasurements EstimateDelayMeasurements(const EnvelopeBuffers &buffers)
{
    std::array<ThresholdCrossing, ADC_CHANNELS> arrivals{};
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        arrivals[channel] = FindThresholdCrossing(buffers[channel]);
    }

    DelayMeasurements current{};
    current.valid = true;
    for (const auto &arrival : arrivals)
    {
        if (!arrival.found)
        {
            current.valid = false;
            break;
        }
    }

    if (!current.valid)
    {
        return current;
    }

    const int32_t ref_index = static_cast<int32_t>(arrivals[0].index);
    current.d12_samples = static_cast<int32_t>(arrivals[1].index) - ref_index;
    current.d13_samples = static_cast<int32_t>(arrivals[2].index) - ref_index;
    current.d14_samples = static_cast<int32_t>(arrivals[3].index) - ref_index;
    current.d12_us = SamplesToMicroseconds(current.d12_samples);
    current.d13_us = SamplesToMicroseconds(current.d13_samples);
    current.d14_us = SamplesToMicroseconds(current.d14_samples);
    return current;
}

float SamplesToMicroseconds(int32_t samples)
{
    const float samples_f = static_cast<float>(samples);
    return (samples_f * kMicrosecondsPerSecond) / SAMPLE_RATE_HZ;
}
} // namespace peleng
