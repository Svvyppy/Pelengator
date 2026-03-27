#include "Peleng.hpp"

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
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        envelope_filters_[channel].ApplyEnvelope(work_buffers_[channel].data() + start_index,
                                                 envelope_buffers_[channel].data(), DMA_HALF_BUFFER_SIZE);
    }

    latest_delays_ = peleng::EstimateDelayMeasurements(envelope_buffers_);
    has_new_delays_ = true;
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
