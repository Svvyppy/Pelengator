#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "CommonSettings.h"

struct DelayMeasurements
{
    int32_t d12_samples = 0;
    int32_t d13_samples = 0;
    int32_t d14_samples = 0;

    float d12_us = 0.0f;
    float d13_us = 0.0f;
    float d14_us = 0.0f;

    bool valid = false;
};

struct ThresholdCrossing
{
    std::size_t index = 0U;
    float value = 0.0f;
    bool found = false;
};

namespace peleng
{
using HalfBuffer = std::array<float, DMA_HALF_BUFFER_SIZE>;
using EnvelopeBuffers = std::array<HalfBuffer, ADC_CHANNELS>;

ThresholdCrossing FindThresholdCrossing(const HalfBuffer &buffer);
DelayMeasurements EstimateDelayMeasurements(const EnvelopeBuffers &buffers);
float SamplesToMicroseconds(int32_t samples);
} // namespace peleng
