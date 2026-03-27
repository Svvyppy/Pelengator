#include "Peleng.hpp"

Peleng::Peleng() = default;

void Peleng::Init()
{
    InitHw();
    InitAdcs();
}

/**
 * @brief Process pending DMA flags and run DSP on ready half-frames.
 */
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

/**
 * @brief Convert one half of each channel and mark half-frame ready for DSP.
 */
void Peleng::DmaHalfTransferCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        ConvertAdcToF32(adc_buffers_[channel].data(), work_buffers_[channel].data(), DMA_HALF_BUFFER_SIZE);
    }
    adc_half_flag_ = true;
}

/**
 * @brief Convert second half of each channel and mark frame ready for DSP.
 */
void Peleng::DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        ConvertAdcToF32(adc_buffers_[channel].data() + DMA_HALF_BUFFER_SIZE,
                        work_buffers_[channel].data() + DMA_HALF_BUFFER_SIZE,
                        DMA_HALF_BUFFER_SIZE);
    }
    adc_full_flag_ = true;
}

bool Peleng::TryGetLatestDelays(DelayMeasurements *out)
{
    return dsp_.TryGetLatestDelays(out);
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

/**
 * @brief Build channel pointer set for one half-frame and run DSP stage.
 */
void Peleng::ProcessHalfTransfer(std::size_t start_index)
{
    PelengDsp::ChannelHalfPointers input_half{};

    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        input_half[channel] = work_buffers_[channel].data() + start_index;
    }

    dsp_.ProcessHalf(input_half);
}

void Peleng::ConvertAdcToF32(const uint32_t *source, float32_t *destination, std::size_t length)
{
    constexpr uint32_t kAdcMask12Bit = 0x0FFFU;
    constexpr int32_t kAdcMidpoint = 2048;
    constexpr int32_t kQ31Scale = 1 << 19; // Keep legacy scaling used by the DSP chain.

    for (std::size_t index = 0U; index < length; ++index)
    {
        const int32_t raw = static_cast<int32_t>(source[index] & kAdcMask12Bit);
        const int32_t centered = raw - kAdcMidpoint;     // [-2048, 2047]
        const int32_t scaled = centered * kQ31Scale;     // [-1073741824, 1073217536]
        destination[index] = static_cast<float32_t>(scaled);
    }
}
