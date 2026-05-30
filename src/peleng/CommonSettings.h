#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @file CommonSettings.h
 * @brief Global DSP and acquisition configuration constants.
 *
 * The constants in this file define buffer geometry, FIR processing granularity,
 * sampling frequency, and detector thresholds used by the entire signal chain.
 */

/** @brief Number of samples processed in one DSP frame per half-transfer. */
static inline constexpr const std::size_t BUFFER_SIZE = 1024U;
/** @brief FIR processing block size expected by CMSIS-DSP wrapper. */
static inline constexpr const std::size_t BLOCK_SIZE = 32U;
/** @brief Number of FIR taps in the envelope filter. */
static inline constexpr const std::size_t NUM_TAPS = 17U;
/** @brief Q15 CMSIS-DSP FIR requires an even number of taps on DSP-enabled Cortex-M builds. */
static inline constexpr const std::size_t Q15_NUM_TAPS = NUM_TAPS + 1U;
/** @brief Historical filter-order constant (kept for compatibility). */
static inline constexpr const std::size_t FIR_ORDER = 16U;
/** @brief Number of BLOCK_SIZE chunks inside one BUFFER_SIZE frame. */
static inline constexpr const std::size_t NUM_BLOCKS = BUFFER_SIZE / BLOCK_SIZE;

/** @brief Number of hydrophone/ADC channels processed in parallel. */
static inline constexpr const std::size_t ADC_CHANNELS = 4U;
/** @brief Envelope threshold for arrival detection in Q15 units. */
static inline constexpr const int16_t SIGNAL_THRESHOLD_Q15 = 4000;
/** @brief ADC sampling frequency in hertz. */
static inline constexpr const float SAMPLE_RATE_HZ = 250000.0f;

/** @brief Samples available on each DMA half-transfer event. */
static inline constexpr const std::size_t DMA_HALF_BUFFER_SIZE = BUFFER_SIZE;
/** @brief Total circular DMA buffer size per channel (half + half). */
static inline constexpr const std::size_t DMA_FULL_BUFFER_SIZE = BUFFER_SIZE * 2U;

/** @brief Distance between 1 and 2 hydrophone. */
static inline constexpr float const Dist1n2 = 0.235;
/** @brief Distance between 1 and 3 hydrophone. */
static inline constexpr float const Dist1n3 = 0.235;
/** @brief Distance between 1 and 4 hydrophone. */
static inline constexpr const float Dist1n4 = 0.235;
