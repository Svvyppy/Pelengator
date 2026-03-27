#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

extern "C"
{
#include "arm_math.h"
}

#include "CommonSettings.h"
#include "Filter.hpp"

/**
 * @brief Output delay set relative to channel 1.
 */
struct DelayMeasurements
{
    /** @brief Delay channel2 - channel1 in samples. */
    int32_t d12_samples = 0;
    /** @brief Delay channel3 - channel1 in samples. */
    int32_t d13_samples = 0;
    /** @brief Delay channel4 - channel1 in samples. */
    int32_t d14_samples = 0;

    /** @brief Delay channel2 - channel1 in microseconds. */
    float32_t d12_us = 0.0f;
    /** @brief Delay channel3 - channel1 in microseconds. */
    float32_t d13_us = 0.0f;
    /** @brief Delay channel4 - channel1 in microseconds. */
    float32_t d14_us = 0.0f;

    /** @brief True when all channels have valid threshold detections. */
    bool valid = false;
};

/**
 * @brief Pure DSP stage: envelope extraction and delay estimation.
 *
 * This class has no hardware dependencies and can be tested on host.
 */
class PelengDsp
{
public:
    using HalfBuffer = std::array<float32_t, DMA_HALF_BUFFER_SIZE>;
    using ChannelHalfPointers = std::array<const float32_t *, ADC_CHANNELS>;

    PelengDsp() = default;

    /**
     * @brief Process one half-frame for all channels.
     * @param input_channels Pointers to channel buffers of DMA_HALF_BUFFER_SIZE each.
     */
    void ProcessHalf(const ChannelHalfPointers &input_channels);

    /**
     * @brief Retrieve latest delays once.
     * @param out Output delay structure.
     * @return true when new data is returned.
     */
    bool TryGetLatestDelays(DelayMeasurements *out);

private:
    struct ThresholdExcess
    {
        std::size_t index = 0U;
        float32_t value = 0.0f;
        bool found = false;
    };

    std::array<HalfBuffer, ADC_CHANNELS> envelope_buffers_{};
    HalfBuffer square_buffer_{};
    std::array<Filter, ADC_CHANNELS> envelope_filters_{};

    DelayMeasurements latest_delays_{};
    bool has_new_delays_ = false;

    /** @brief Find first envelope sample above SIGNAL_THRESHOLD. */
    static ThresholdExcess FindThresholdExcess(const HalfBuffer &buffer);
    /** @brief Convert delay in samples to microseconds using SAMPLE_RATE_HZ. */
    static float32_t SamplesToMicroseconds(int32_t samples);
};
