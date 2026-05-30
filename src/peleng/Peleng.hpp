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
    struct DebugSnapshot
    {
        std::array<uint16_t, ADC_CHANNELS> first_samples{};
        std::array<uint32_t, ADC_CHANNELS> dma_cndtr{};
        std::array<uint32_t, ADC_CHANNELS> adc_dr{};
        std::array<uint32_t, ADC_CHANNELS> adc_isr{};
        std::array<uint32_t, ADC_CHANNELS> adc_cr{};
        std::array<uint32_t, ADC_CHANNELS> half_counts{};
        std::array<uint32_t, ADC_CHANNELS> full_counts{};
    };

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
    void FillDebugSnapshot(DebugSnapshot *out) const;

private:
    // ADC DMA is configured for halfword transfers, so each sample occupies 16 bits in RAM.
    using AdcDmaBuffer = std::array<uint16_t, DMA_FULL_BUFFER_SIZE>;
    using HalfBuffer = std::array<q15_t, DMA_HALF_BUFFER_SIZE>;

    std::array<AdcDmaBuffer, ADC_CHANNELS> adc_buffers_{};
    std::array<HalfBuffer, ADC_CHANNELS> work_buffers_{};
    std::array<HalfBuffer, ADC_CHANNELS> envelope_buffers_{};
    std::array<Filter, ADC_CHANNELS> envelope_filters_{};

    DelayMeasurements latest_delays_{};
    bool has_new_delays_ = false;

    volatile bool adc_half_flag_ = false;
    volatile bool adc_full_flag_ = false;
    std::array<uint32_t, ADC_CHANNELS> dma_half_counts_{};
    std::array<uint32_t, ADC_CHANNELS> dma_full_counts_{};

    void InitAdcs();
    void ProcessHalfTransfer(std::size_t start_index);
    static void ConvertAdcToQ15(const uint16_t *source, q15_t *destination, std::size_t length);
    static std::size_t AdcHandleToIndex(const ADC_HandleTypeDef *hadc);
};

uint32_t PelengGetDmaHalfCount(void);
uint32_t PelengGetDmaFullCount(void);
