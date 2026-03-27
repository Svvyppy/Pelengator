#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

extern "C"
{
#include "arm_math.h"
}

#include "Hw.h"
#include "dsp/PelengDsp.hpp"

/**
 * @brief Top-level acquisition and DSP orchestrator.
 *
 * Responsibilities:
 * - initialize hardware and DMA acquisition,
 * - react to half/full DMA transfer events,
 * - convert raw ADC data to float domain,
 * - invoke DSP processing for each acquired half-buffer.
 */
class Peleng
{
public:
    /** @brief Construct processing object with zero-initialized internal buffers. */
    Peleng();
    ~Peleng() = default;

    /** @brief Initialize hardware and start ADC DMA streaming. */
    void Init();

    /**
     * @brief Process pending DMA half/full events.
     *
     * Call from the main loop as frequently as possible.
     */
    void Process();

    /** @brief Handle ADC DMA half-transfer event. */
    void DmaHalfTransferCallback(ADC_HandleTypeDef *hadc);
    /** @brief Handle ADC DMA transfer-complete event. */
    void DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc);

    /**
     * @brief Get latest computed inter-channel delays, if available.
     * @param out Output structure for delays in samples and microseconds.
     * @return true if a fresh frame is available, false otherwise.
     */
    bool TryGetLatestDelays(DelayMeasurements *out);

private:
    using AdcDmaBuffer = std::array<uint32_t, DMA_FULL_BUFFER_SIZE>;
    using WorkingBuffer = std::array<float32_t, DMA_FULL_BUFFER_SIZE>;

    std::array<AdcDmaBuffer, ADC_CHANNELS> adc_buffers_{};
    std::array<WorkingBuffer, ADC_CHANNELS> work_buffers_{};

    PelengDsp dsp_{};

    volatile bool adc_half_flag_ = false;
    volatile bool adc_full_flag_ = false;

    /** @brief Start all ADC DMA streams and launch trigger timer. */
    void InitAdcs();
    /** @brief Process one DMA half-frame pointed by start index. */
    void ProcessHalfTransfer(std::size_t start_index);

    /**
     * @brief Convert packed ADC samples to centered/scaled float samples.
     *
     * Input is treated as 12-bit right-aligned ADC code. Conversion preserves
     * legacy scaling used by the DSP path.
     */
    static void ConvertAdcToF32(const uint32_t *source, float32_t *destination, std::size_t length);
};
