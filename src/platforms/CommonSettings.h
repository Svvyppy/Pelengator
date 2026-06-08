#pragma once

#include <cstddef>
#include <cstdint>

namespace hydrv {

inline constexpr std::size_t kSignalBlockSize        = 1024U;
inline constexpr std::size_t kFilterBlockSize        = 32U;
inline constexpr std::size_t kEnvelopeFilterTapCount = 18U;
inline constexpr std::size_t kAdcChannelCount        = 4U;
inline constexpr int16_t kSignalThreshold            = 700;
inline constexpr int16_t kMaximumSignalThreshold     = 1000;
inline constexpr float kSampleRateHz                 = 250000.0f;
inline constexpr float kSoundSpeedMetersPerSecond    = 1441.0f;
inline constexpr int32_t kSignalDelayMarginSamples   = 16;
inline constexpr std::size_t kDmaBufferSize          = kSignalBlockSize * 2U;

inline constexpr float kHydrophonePositionsMeters[kAdcChannelCount][3] = {
    { 0.0625f,  0.1175f,   0.0f },
    { 0.0625f, -0.1175f,   0.0f },
    {    0.0f,  -0.037f, 0.220f },
    { -0.295f,     0.0f,   0.0f },
};

} // namespace hydrv
