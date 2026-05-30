#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "CommonSettings.h"

namespace platform {

enum class SignalBlock
{
    FirstHalf,
    SecondHalf,
};

using SignalSampleBuffer = std::array<uint16_t, SIGNAL_SAMPLE_BUFFER_SIZE>;
using SignalSampleBuffers = std::array<SignalSampleBuffer, ADC_CHANNELS>;

struct SignalAcquisitionDebug
{
    std::array<uint16_t, ADC_CHANNELS> first_samples{};
    std::array<uint32_t, ADC_CHANNELS> data_registers{};
    std::array<uint32_t, ADC_CHANNELS> status_registers{};
    std::array<uint32_t, ADC_CHANNELS> control_registers{};
    std::array<uint32_t, ADC_CHANNELS> remaining_transfers{};
    std::array<uint32_t, ADC_CHANNELS> first_half_counts{};
    std::array<uint32_t, ADC_CHANNELS> second_half_counts{};
};

void StartSignalAcquisition(SignalSampleBuffers* buffers);
bool ConsumeReadySignalBlock(SignalBlock* block);
void FillSignalAcquisitionDebug(SignalAcquisitionDebug* out, const SignalSampleBuffers& buffers);
uint32_t GetSignalFirstHalfReadyCount();
uint32_t GetSignalSecondHalfReadyCount();

} // namespace platform
