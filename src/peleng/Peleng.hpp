#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "DelayEstimator.hpp"
#include "Filter.hpp"
#include "Hw.h"

class Peleng
{
public:
    Peleng();
    ~Peleng() = default;

    void Init();
    void Process();

    void DmaHalfTransferCallback(ADC_HandleTypeDef *hadc);
    void DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc);

    bool TryGetLatestDelays(DelayMeasurements *out);

private:
    using AdcDmaBuffer = std::array<uint32_t, DMA_FULL_BUFFER_SIZE>;
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
    static void ConvertAdcToF32(const uint32_t *source, float *destination, std::size_t length);
};
