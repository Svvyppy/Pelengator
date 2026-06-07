#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "DelayEstimator.hpp"
#include "Filter.hpp"
#include "SignalAcquisition.h"

/**
 * @brief Top-level real-time orchestrator for Peleng DSP pipeline.
 *
 * Owns acquisition sample buffers, performs conversion and filtering, estimates
 * delays, and publishes most recent measurements for telemetry.
 */
class Peleng
{
public:
    /** @brief Construct processing pipeline and internal state. */
    Peleng();
    ~Peleng() = default;

    /** @brief Initialize signal acquisition path. */
    void Init();
    /**
     * @brief Run one non-blocking processing iteration in the main loop.
     * @return true when a ready acquisition block was processed.
     */
    bool Process();

    /**
     * @brief Get latest computed channel delays.
     * @param[out] out Destination structure.
     * @return true if a new delay frame was available and copied.
     */
    bool TryGetLatestDelays(DelayMeasurements* out);
    void FillDebugSnapshot(platform::SignalAcquisitionDebug* out) const;

private:
    using HalfBuffer = std::array<q15_t, SIGNAL_BLOCK_SIZE>;

    platform::SignalSampleBuffers sample_buffers_{};
    std::array<HalfBuffer, ADC_CHANNELS> work_buffers_{};
    std::array<HalfBuffer, ADC_CHANNELS> envelope_buffers_{};
    std::array<Filter, ADC_CHANNELS> envelope_filters_{};

    DelayMeasurements latest_delays_{};
    bool has_new_delays_ = false;

    void ProcessHalfTransfer(std::size_t start_index);
    static void ConvertAdcToSquaredQ15(const uint16_t* source, q15_t* destination, std::size_t length);
};
