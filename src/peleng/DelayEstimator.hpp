#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "CommonSettings.h"

/**
 * @brief Relative delays between channel 1 and channels 2..4.
 */
struct DelayMeasurements
{
    /** @brief Delay 2-1 in samples. */
    int32_t d12_samples = 0;
    /** @brief Delay 3-1 in samples. */
    int32_t d13_samples = 0;
    /** @brief Delay 4-1 in samples. */
    int32_t d14_samples = 0;

    /** @brief Delay 2-1 in microseconds. */
    float d12_us = 0.0f;
    /** @brief Delay 3-1 in microseconds. */
    float d13_us = 0.0f;
    /** @brief Delay 4-1 in microseconds. */
    float d14_us = 0.0f;

    /** @brief True when all required threshold crossings were found. */
    bool valid = false;
};

/**
 * @brief First threshold crossing info for one envelope buffer.
 */
struct ThresholdCrossing
{
    /** @brief Sample index where threshold crossing was detected. */
    std::size_t index = 0U;
    /** @brief Envelope value at crossing index. */
    float value = 0.0f;
    /** @brief Indicates whether a crossing was found. */
    bool found = false;
};

namespace peleng
{
/** @brief Half-frame envelope buffer type for one channel. */
using HalfBuffer = std::array<float, DMA_HALF_BUFFER_SIZE>;
/** @brief Envelope buffers for all channels in one processing step. */
using EnvelopeBuffers = std::array<HalfBuffer, ADC_CHANNELS>;

/**
 * @brief Find first threshold crossing in one channel envelope.
 */
ThresholdCrossing FindThresholdCrossing(const HalfBuffer &buffer);
/**
 * @brief Compute inter-channel delays from current envelope buffers.
 */
DelayMeasurements EstimateDelayMeasurements(const EnvelopeBuffers &buffers);
/**
 * @brief Convert delay in samples to microseconds using configured sample rate.
 */
float SamplesToMicroseconds(int32_t samples);
} // namespace peleng
