#include "SignalAcquisition.hpp"

#include <array>

#include "Hw.h"
#include "HwInit.h"

namespace
{
platform::SignalSampleBuffers* g_buffers = nullptr;
volatile bool g_first_half_ready = false;
volatile bool g_second_half_ready = false;
volatile uint32_t g_first_half_count = 0U;
volatile uint32_t g_second_half_count = 0U;
std::array<uint32_t, ADC_CHANNELS> g_first_half_counts{};
std::array<uint32_t, ADC_CHANNELS> g_second_half_counts{};

void CheckHalStatus(HAL_StatusTypeDef status)
{
    if (status != HAL_OK)
    {
        Error_Handler();
    }
}

std::size_t AdcHandleToIndex(const ADC_HandleTypeDef* hadc)
{
    if (hadc == nullptr)
    {
        return ADC_CHANNELS;
    }

    if (hadc->Instance == ADC4)
    {
        return 0U;
    }
    if (hadc->Instance == ADC5)
    {
        return 1U;
    }
    if (hadc->Instance == ADC2)
    {
        return 2U;
    }
    if (hadc->Instance == ADC1)
    {
        return 3U;
    }

    return ADC_CHANNELS;
}

void NoteFirstHalfReady(ADC_HandleTypeDef* hadc)
{
    const std::size_t index = AdcHandleToIndex(hadc);
    if (index < ADC_CHANNELS)
    {
        ++g_first_half_counts[index];
    }
    ++g_first_half_count;
    g_first_half_ready = true;
}

void NoteSecondHalfReady(ADC_HandleTypeDef* hadc)
{
    const std::size_t index = AdcHandleToIndex(hadc);
    if (index < ADC_CHANNELS)
    {
        ++g_second_half_counts[index];
    }
    ++g_second_half_count;
    g_second_half_ready = true;
}
} // namespace

namespace platform {

void StartSignalAcquisition(SignalSampleBuffers* buffers)
{
    if (buffers == nullptr)
    {
        Error_Handler();
    }

    g_buffers = buffers;

    HwInstances* hw = GetHwInstances();
    HAL_TIM_Base_Stop(&hw->htim6);
    CheckHalStatus(HAL_DAC_Start(&hw->hdac4, DAC_CHANNEL_1));
    CheckHalStatus(HAL_DAC_SetValue(&hw->hdac4, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048));
    CheckHalStatus(HAL_OPAMP_Start(&hw->hopamp4));

    CheckHalStatus(HAL_ADC_Start_DMA(&hw->hadc4, reinterpret_cast<uint32_t*>((*g_buffers)[0].data()),
                                     SIGNAL_SAMPLE_BUFFER_SIZE));
    CheckHalStatus(HAL_ADC_Start_DMA(&hw->hadc5, reinterpret_cast<uint32_t*>((*g_buffers)[1].data()),
                                     SIGNAL_SAMPLE_BUFFER_SIZE));
    CheckHalStatus(HAL_ADC_Start_DMA(&hw->hadc2, reinterpret_cast<uint32_t*>((*g_buffers)[2].data()),
                                     SIGNAL_SAMPLE_BUFFER_SIZE));
    CheckHalStatus(HAL_ADC_Start_DMA(&hw->hadc1, reinterpret_cast<uint32_t*>((*g_buffers)[3].data()),
                                     SIGNAL_SAMPLE_BUFFER_SIZE));
    CheckHalStatus(HAL_TIM_Base_Start(&hw->htim6));
    CheckHalStatus(HAL_TIM_Base_Start(&hw->htim7));
}

bool ConsumeReadySignalBlock(SignalBlock* block)
{
    if (block == nullptr)
    {
        return false;
    }

    if (g_first_half_ready)
    {
        g_first_half_ready = false;
        *block = SignalBlock::FirstHalf;
        return true;
    }

    if (g_second_half_ready)
    {
        g_second_half_ready = false;
        *block = SignalBlock::SecondHalf;
        return true;
    }

    return false;
}

void FillSignalAcquisitionDebug(SignalAcquisitionDebug* out, const SignalSampleBuffers& buffers)
{
    if (out == nullptr)
    {
        return;
    }

    const HwInstances* hw = GetHwInstances();
    const std::array<const ADC_HandleTypeDef*, ADC_CHANNELS> adcs = {&hw->hadc4, &hw->hadc5, &hw->hadc2,
                                                                     &hw->hadc1};

    for (std::size_t i = 0U; i < ADC_CHANNELS; ++i)
    {
        const ADC_HandleTypeDef* hadc = adcs[i];
        out->first_samples[i] = buffers[i][0];
        out->data_registers[i] = hadc->Instance->DR;
        out->status_registers[i] = hadc->Instance->ISR;
        out->control_registers[i] = hadc->Instance->CR;
        out->first_half_counts[i] = g_first_half_counts[i];
        out->second_half_counts[i] = g_second_half_counts[i];
        out->remaining_transfers[i] = (hadc->DMA_Handle != nullptr) ? hadc->DMA_Handle->Instance->CNDTR : 0U;
    }
}

uint32_t GetSignalFirstHalfReadyCount() { return g_first_half_count; }

uint32_t GetSignalSecondHalfReadyCount() { return g_second_half_count; }

} // namespace platform

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    NoteSecondHalfReady(hadc);
}

extern "C" void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    NoteFirstHalfReady(hadc);
}
