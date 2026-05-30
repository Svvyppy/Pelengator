#pragma once

#include <array>
#include <cstddef>

extern "C"
{
#include <arm_math.h>
}

#include "CommonSettings.h"
#include "FilterCoefficients.hpp"

/**
 * @brief Lightweight FIR wrapper over CMSIS-DSP arm_fir_q15.
 *
 * The class owns filter state memory and applies FIR in fixed-size blocks.
 * It is intentionally allocation-free and suitable for deterministic embedded DSP.
 */
class Filter
{
public:
    /** @brief Initialize FIR instance and internal state. */
    Filter();

    /**
     * @brief Apply FIR to a contiguous input buffer.
     * @param input Pointer to input samples.
     * @param output Pointer to output samples.
     * @param sample_count Number of samples to process.
     * @note sample_count must be a multiple of BLOCK_SIZE.
     */
    void ApplyEnvelope(const q15_t *input, q15_t *output, std::size_t sample_count);

private:
    arm_fir_instance_q15 fir_instance_{};
    std::array<q15_t, DMA_HALF_BUFFER_SIZE> square_buffer_{};
    std::array<q15_t, BLOCK_SIZE + Q15_NUM_TAPS> state_{};
};
