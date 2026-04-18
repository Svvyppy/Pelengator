#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "DelayEstimator.hpp"
#include "Filter.hpp"
#include "Hw.h"

/**
 * @brief Top-level real-time orchestrator for Peleng DSP pipeline.
 *
 * Owns ADC DMA buffers, performs conversion and filtering, estimates delays,
 * and publishes most recent measurements for telemetry.
 */
class Peleng
{
public:
    /** @brief Construct processing pipeline and internal state. */
    Peleng();
    ~Peleng() = default;

    /** @brief Initialize hardware-facing ADC acquisition path. */
    void Init();
    /** @brief Run one non-blocking processing iteration in the main loop. */
    void Process();

    /** @brief Mark first half of DMA buffers ready for processing. */
    void DmaHalfTransferCallback(ADC_HandleTypeDef *hadc);
    /** @brief Mark second half of DMA buffers ready for processing. */
    void DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc);

    /**
     * @brief Get latest computed channel delays.
     * @param[out] out Destination structure.
     * @return true if a new delay frame was available and copied.
     */
    bool TryGetLatestDelays(DelayMeasurements *out);

private:
    // ADC DMA is configured for halfword transfers, so each sample occupies 16 bits in RAM.
    using AdcDmaBuffer = std::array<uint16_t, DMA_FULL_BUFFER_SIZE>;
    using WorkingBuffer = std::array<float, DMA_FULL_BUFFER_SIZE>;
    using HalfBuffer = std::array<float, DMA_HALF_BUFFER_SIZE>;

    std::array<AdcDmaBuffer, ADC_CHANNELS> adc_buffers_{};
    std::array<WorkingBuffer, ADC_CHANNELS> work_buffers_{};
    std::array<HalfBuffer, ADC_CHANNELS> envelope_buffers_{};
    std::array<Filter, ADC_CHANNELS> envelope_filters_{};

    DelayMeasurements latest_delays_{};
    bool has_new_delays_ = false;

    volatile bool adc_half_flag_ = false;
    volatile bool adc_full_flag_ = false;

    void InitAdcs();
    void ProcessHalfTransfer(std::size_t start_index);
    static void ConvertAdcToF32(const uint16_t *source, float *destination, std::size_t length);
};

uint32_t PelengGetDmaHalfCount(void);
uint32_t PelengGetDmaFullCount(void);
