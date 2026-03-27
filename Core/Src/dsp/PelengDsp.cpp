#include "dsp/PelengDsp.hpp"

namespace
{
constexpr float32_t kMicrosecondsPerSecond = 1000000.0f;
}

/**
 * @brief Envelope/delay processing for one half-buffer across all channels.
 *
 * Processing steps per channel:
 * 1) square input samples,
 * 2) run FIR envelope filter with per-channel state,
 * 3) detect first threshold crossing.
 */
void PelengDsp::ProcessHalf(const ChannelHalfPointers &input_channels)
{
    std::array<ThresholdExcess, ADC_CHANNELS> arrivals{};

    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        arm_mult_f32(input_channels[channel],
                     input_channels[channel],
                     square_buffer_.data(),
                     DMA_HALF_BUFFER_SIZE);

        envelope_filters_[channel].Apply(square_buffer_.data(), envelope_buffers_[channel].data(), DMA_HALF_BUFFER_SIZE);
        arrivals[channel] = FindThresholdExcess(envelope_buffers_[channel]);
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

    if (current.valid)
    {
        const int32_t ref_index = static_cast<int32_t>(arrivals[0].index);

        current.d12_samples = static_cast<int32_t>(arrivals[1].index) - ref_index;
        current.d13_samples = static_cast<int32_t>(arrivals[2].index) - ref_index;
        current.d14_samples = static_cast<int32_t>(arrivals[3].index) - ref_index;

        current.d12_us = SamplesToMicroseconds(current.d12_samples);
        current.d13_us = SamplesToMicroseconds(current.d13_samples);
        current.d14_us = SamplesToMicroseconds(current.d14_samples);
    }

    latest_delays_ = current;
    has_new_delays_ = true;
}

bool PelengDsp::TryGetLatestDelays(DelayMeasurements *out)
{
    if (!has_new_delays_ || out == nullptr)
    {
        return false;
    }

    *out = latest_delays_;
    has_new_delays_ = false;
    return true;
}

/**
 * @brief First-threshold detector used for coarse arrival-time estimation.
 */
PelengDsp::ThresholdExcess PelengDsp::FindThresholdExcess(const HalfBuffer &buffer)
{
    for (std::size_t index = 0U; index < DMA_HALF_BUFFER_SIZE; ++index)
    {
        const float32_t value = buffer[index];
        if (value > SIGNAL_THRESHOLD)
        {
            return ThresholdExcess{index, value, true};
        }
    }

    return ThresholdExcess{};
}

float32_t PelengDsp::SamplesToMicroseconds(int32_t samples)
{
    const float32_t samples_f = static_cast<float32_t>(samples);
    return (samples_f * kMicrosecondsPerSecond) / SAMPLE_RATE_HZ;
}
