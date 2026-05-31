#include "SignalAcquisition.hpp"

#include <array>

#include "Hw.h"
#include "HwInit.h"

namespace
{
constexpr uint32_t kReadyBlockQueueDepth = 4U;

platform::SignalSampleBuffers* g_buffers = nullptr;
std::array<volatile uint32_t, ADC_CHANNELS> g_first_half_counts{};
std::array<volatile uint32_t, ADC_CHANNELS> g_second_half_counts{};
volatile uint32_t g_first_half_completed = 0U;
volatile uint32_t g_second_half_completed = 0U;
volatile platform::SignalBlock g_ready_blocks[kReadyBlockQueueDepth] = {};
volatile uint32_t g_ready_block_head = 0U;
volatile uint32_t g_ready_block_count = 0U;
volatile uint32_t g_ready_block_overflow_count = 0U;

uint32_t EnterCritical(void)
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void ExitCritical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

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

uint32_t MinimumCount(const std::array<volatile uint32_t, ADC_CHANNELS>& counts)
{
    uint32_t minimum = counts[0];
    for (std::size_t i = 1U; i < ADC_CHANNELS; ++i)
    {
        if (counts[i] < minimum)
        {
            minimum = counts[i];
        }
    }

    return minimum;
}

void EnqueueReadyBlock(platform::SignalBlock block)
{
    if (g_ready_block_count >= kReadyBlockQueueDepth)
    {
        ++g_ready_block_overflow_count;
        return;
    }

    const uint32_t slot = (g_ready_block_head + g_ready_block_count) % kReadyBlockQueueDepth;
    g_ready_blocks[slot] = block;
    ++g_ready_block_count;
}

void PublishCompletedBlocks(const std::array<volatile uint32_t, ADC_CHANNELS>& counts,
                            volatile uint32_t* completed_count,
                            platform::SignalBlock block)
{
    const uint32_t complete_count = MinimumCount(counts);
    while (*completed_count < complete_count)
    {
        ++(*completed_count);
        EnqueueReadyBlock(block);
    }
}

void ResetReadyState()
{
    for (std::size_t i = 0U; i < ADC_CHANNELS; ++i)
    {
        g_first_half_counts[i] = 0U;
        g_second_half_counts[i] = 0U;
    }

    g_first_half_completed = 0U;
    g_second_half_completed = 0U;
    g_ready_block_head = 0U;
    g_ready_block_count = 0U;
    g_ready_block_overflow_count = 0U;
}

void NoteFirstHalfReady(ADC_HandleTypeDef* hadc)
{
    const std::size_t index = AdcHandleToIndex(hadc);
    if (index >= ADC_CHANNELS)
    {
        return;
    }

    ++g_first_half_counts[index];
    PublishCompletedBlocks(g_first_half_counts, &g_first_half_completed, platform::SignalBlock::FirstHalf);
}

void NoteSecondHalfReady(ADC_HandleTypeDef* hadc)
{
    const std::size_t index = AdcHandleToIndex(hadc);
    if (index >= ADC_CHANNELS)
    {
        return;
    }

    ++g_second_half_counts[index];
    PublishCompletedBlocks(g_second_half_counts, &g_second_half_completed, platform::SignalBlock::SecondHalf);
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
    ResetReadyState();

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

    const uint32_t primask = EnterCritical();
    if (g_ready_block_count == 0U)
    {
        ExitCritical(primask);
        return false;
    }

    *block = g_ready_blocks[g_ready_block_head];
    g_ready_block_head = (g_ready_block_head + 1U) % kReadyBlockQueueDepth;
    --g_ready_block_count;
    ExitCritical(primask);
    return true;
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
    out->ready_blocks_pending = g_ready_block_count;
    out->ready_block_overflows = g_ready_block_overflow_count;
}

uint32_t GetSignalFirstHalfReadyCount() { return g_first_half_completed; }

uint32_t GetSignalSecondHalfReadyCount() { return g_second_half_completed; }

} // namespace platform

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    NoteSecondHalfReady(hadc);
}

extern "C" void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    NoteFirstHalfReady(hadc);
}
