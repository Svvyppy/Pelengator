#include "Peleng.hpp"

namespace
{
constexpr float kMicrosecondsPerSecond = 1000000.0f;
}

Peleng::Peleng() = default;

void Peleng::Init() { InitAdcs(); }

void Peleng::Process()
{
    if (adc_half_flag_)
    {
        ProcessHalfTransfer(0U);
        adc_half_flag_ = false;
    }

    if (adc_full_flag_)
    {
        ProcessHalfTransfer(DMA_HALF_BUFFER_SIZE);
        adc_full_flag_ = false;
    }
}

void Peleng::DmaHalfTransferCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        ConvertAdcToF32(adc_buffers_[channel].data(), work_buffers_[channel].data(), DMA_HALF_BUFFER_SIZE);
    }
    adc_half_flag_ = true;
}

void Peleng::DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        ConvertAdcToF32(adc_buffers_[channel].data() + DMA_HALF_BUFFER_SIZE,
                        work_buffers_[channel].data() + DMA_HALF_BUFFER_SIZE, DMA_HALF_BUFFER_SIZE);
    }
    adc_full_flag_ = true;
}

bool Peleng::TryGetLatestDelays(DelayMeasurements *out)
{
    if (!has_new_delays_ || out == nullptr)
    {
        return false;
    }

    *out = latest_delays_;
    has_new_delays_ = false;
    return true;
}

void Peleng::InitAdcs()
{
    HAL_TIM_Base_Stop(&GetHwInstances()->htim6);
    HAL_ADC_Start_DMA(&GetHwInstances()->hadc1, adc_buffers_[0].data(), DMA_FULL_BUFFER_SIZE);
    HAL_ADC_Start_DMA(&GetHwInstances()->hadc2, adc_buffers_[1].data(), DMA_FULL_BUFFER_SIZE);
    HAL_ADC_Start_DMA(&GetHwInstances()->hadc3, adc_buffers_[2].data(), DMA_FULL_BUFFER_SIZE);
    HAL_ADC_Start_DMA(&GetHwInstances()->hadc5, adc_buffers_[3].data(), DMA_FULL_BUFFER_SIZE);
    HAL_TIM_Base_Start(&GetHwInstances()->htim6);
}

void Peleng::ProcessHalfTransfer(std::size_t start_index)
{
    std::array<ThresholdExcess, ADC_CHANNELS> arrivals{};

    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        envelope_filters_[channel].ApplyEnvelope(work_buffers_[channel].data() + start_index,
                                                 envelope_buffers_[channel].data(), DMA_HALF_BUFFER_SIZE);
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

Peleng::ThresholdExcess Peleng::FindThresholdExcess(const HalfBuffer &buffer)
{
    for (std::size_t index = 0U; index < DMA_HALF_BUFFER_SIZE; ++index)
    {
        const float value = buffer[index];
        if (value > SIGNAL_THRESHOLD)
        {
            return ThresholdExcess{index, value, true};
        }
    }

    return ThresholdExcess{};
}

float Peleng::SamplesToMicroseconds(int32_t samples)
{
    const float samples_f = static_cast<float>(samples);
    return (samples_f * kMicrosecondsPerSecond) / SAMPLE_RATE_HZ;
}

void Peleng::ConvertAdcToF32(const uint32_t *source, float *destination, std::size_t length)
{
    constexpr uint32_t kAdcMask12Bit = 0x0FFFU;
    constexpr int32_t kAdcMidpoint = 2048;
    constexpr int32_t kQ31Scale = 1 << 19;

    for (std::size_t index = 0U; index < length; ++index)
    {
        const int32_t raw = static_cast<int32_t>(source[index] & kAdcMask12Bit);
        const int32_t centered = raw - kAdcMidpoint;
        const int32_t scaled = centered * kQ31Scale;
        destination[index] = static_cast<float>(scaled);
    }
}
