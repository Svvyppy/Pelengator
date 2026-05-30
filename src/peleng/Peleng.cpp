#include "Peleng.hpp"

#include <cstdio>

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
    const std::size_t index = AdcHandleToIndex(hadc);
    if (index < ADC_CHANNELS)
    {
        ++dma_half_counts_[index];
    }
    ++g_dma_half_count;
    adc_half_flag_ = true;
}

void Peleng::DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc)
{
    const std::size_t index = AdcHandleToIndex(hadc);
    if (index < ADC_CHANNELS)
    {
        ++dma_full_counts_[index];
    }
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

void Peleng::FillDebugSnapshot(DebugSnapshot *out) const
{
    if (out == nullptr)
    {
        return;
    }

    const HwInstances *hw = GetHwInstances();
    const std::array<const ADC_HandleTypeDef *, ADC_CHANNELS> adcs = {&hw->hadc1, &hw->hadc2, &hw->hadc4, &hw->hadc5};

    for (std::size_t i = 0U; i < ADC_CHANNELS; ++i)
    {
        const ADC_HandleTypeDef *hadc = adcs[i];
        out->first_samples[i] = adc_buffers_[i][0];
        out->adc_dr[i] = hadc->Instance->DR;
        out->adc_isr[i] = hadc->Instance->ISR;
        out->adc_cr[i] = hadc->Instance->CR;
        out->half_counts[i] = dma_half_counts_[i];
        out->full_counts[i] = dma_full_counts_[i];
        out->dma_cndtr[i] = (hadc->DMA_Handle != nullptr) ? hadc->DMA_Handle->Instance->CNDTR : 0U;
    }
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
    HAL_TIM_Base_Start(&GetHwInstances()->htim7);
}

void Peleng::ProcessHalfTransfer(std::size_t start_index)
{
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        ConvertAdcToQ15(adc_buffers_[channel].data() + start_index, work_buffers_[channel].data(),
                        DMA_HALF_BUFFER_SIZE);
        envelope_filters_[channel].ApplyEnvelope(work_buffers_[channel].data(), envelope_buffers_[channel].data(),
                                                 DMA_HALF_BUFFER_SIZE);
    }

    latest_delays_ = peleng::EstimateDelayMeasurements(envelope_buffers_);
    has_new_delays_ = true;
}

void Peleng::ConvertAdcToQ15(const uint16_t *source, q15_t *destination, std::size_t length)
{
    constexpr uint32_t kAdcMask12Bit = 0x0FFFU;
    constexpr int32_t kAdcMidpoint = 2048;
    constexpr int32_t kQ15Shift = 4;

    for (std::size_t index = 0U; index < length; ++index)
    {
        const int32_t raw = static_cast<int32_t>(source[index] & kAdcMask12Bit);
        const int32_t centered = raw - kAdcMidpoint;
        destination[index] = static_cast<q15_t>(centered << kQ15Shift);
    }
}

uint32_t PelengGetDmaHalfCount(void) { return g_dma_half_count; }

uint32_t PelengGetDmaFullCount(void) { return g_dma_full_count; }

std::size_t Peleng::AdcHandleToIndex(const ADC_HandleTypeDef *hadc)
{
    if (hadc == nullptr)
    {
        return ADC_CHANNELS;
    }

    if (hadc->Instance == ADC1)
    {
        return 0U;
    }
    if (hadc->Instance == ADC2)
    {
        return 1U;
    }
    if (hadc->Instance == ADC4)
    {
        return 2U;
    }
    if (hadc->Instance == ADC5)
    {
        return 3U;
    }

    return ADC_CHANNELS;
}
