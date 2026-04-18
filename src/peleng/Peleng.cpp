#include "Peleng.hpp"

#include "HwInit.h"
#include "UartTelemetry.hpp"

namespace
{
void CheckHalStatus(HAL_StatusTypeDef status)
{
    if (status != HAL_OK)
    {
        Error_Handler();
    }
}

volatile uint32_t g_dma_half_count = 0U;
volatile uint32_t g_dma_full_count = 0U;
} // namespace

Peleng::Peleng() = default;

void Peleng::Init()
{
    event << "Peleng Init";
    adc_buffers_[0].fill(0);
    adc_buffers_[1].fill(0);
    adc_buffers_[2].fill(0);
    adc_buffers_[3].fill(0);
    InitAdcs();
    event << "ADC DMA Started";
}

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


    ++g_dma_half_count;
    adc_half_flag_ = true;
}

void Peleng::DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc)
{


    ++g_dma_full_count;
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
    HAL_DAC_Start(&GetHwInstances()->hdac4, DAC_CHANNEL_1);


    HAL_DAC_SetValue(&GetHwInstances()->hdac4, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048);


    HAL_OPAMP_Start(&GetHwInstances()->hopamp4);


    CheckHalStatus(HAL_ADC_Start_DMA(&GetHwInstances()->hadc1, reinterpret_cast<uint32_t *>(adc_buffers_[0].data()),
                                     DMA_FULL_BUFFER_SIZE));
    CheckHalStatus(HAL_ADC_Start_DMA(&GetHwInstances()->hadc2, reinterpret_cast<uint32_t *>(adc_buffers_[1].data()),
                                     DMA_FULL_BUFFER_SIZE));
    CheckHalStatus(HAL_ADC_Start_DMA(&GetHwInstances()->hadc4, reinterpret_cast<uint32_t *>(adc_buffers_[2].data()),
                                     DMA_FULL_BUFFER_SIZE));
    CheckHalStatus(HAL_ADC_Start_DMA(&GetHwInstances()->hadc5, reinterpret_cast<uint32_t *>(adc_buffers_[3].data()),
                                     DMA_FULL_BUFFER_SIZE));
    HAL_TIM_Base_Start(&GetHwInstances()->htim6);
}

void Peleng::ProcessHalfTransfer(std::size_t start_index)
{
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        ConvertAdcToF32(adc_buffers_[channel].data() + start_index, work_buffers_[channel].data() + start_index,
                        DMA_HALF_BUFFER_SIZE);
        envelope_filters_[channel].ApplyEnvelope(work_buffers_[channel].data() + start_index,
                                                 envelope_buffers_[channel].data(), DMA_HALF_BUFFER_SIZE);
    }

    latest_delays_ = peleng::EstimateDelayMeasurements(envelope_buffers_);
    has_new_delays_ = true;
}

void Peleng::ConvertAdcToF32(const uint16_t *source, float *destination, std::size_t length)
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

uint32_t PelengGetDmaHalfCount(void) { return g_dma_half_count; }

uint32_t PelengGetDmaFullCount(void) { return g_dma_full_count; }
