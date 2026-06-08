#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

extern "C"
{
#include <arm_math.h>
}

#include "CommonSettings.h"

namespace hydrv::peleng {

struct DelayMeasurements
{
    int32_t channel2Samples    = 0;
    int32_t channel3Samples    = 0;
    int32_t channel4Samples    = 0;
    float channel2Microseconds = 0.0f;
    float channel3Microseconds = 0.0f;
    float channel4Microseconds = 0.0f;
    float directionX           = 0.0f;
    float directionY           = 0.0f;
    float directionZ           = 0.0f;
    float bearingDegrees       = 0.0f;
    float elevationDegrees     = 0.0f;
    bool valid                 = false;
    bool directionValid        = false;
};

struct ThresholdCrossing
{
    std::size_t sampleIndex = 0U;
    q15_t value             = 0;
    bool found              = false;
};

using SignalBlock         = std::array<q15_t, kSignalBlockSize>;
using ChannelSignalBlocks = std::array<SignalBlock, kAdcChannelCount>;

ThresholdCrossing findThresholdCrossing(const SignalBlock& signal);
DelayMeasurements estimateDelays(const ChannelSignalBlocks& signals);
float samplesToMicroseconds(int32_t samples);
void estimateDirection(DelayMeasurements& measurements);

} // namespace hydrv::peleng
