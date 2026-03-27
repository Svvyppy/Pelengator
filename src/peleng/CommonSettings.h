#pragma once

#include <cstddef>

/**
 * @file CommonSettings.h
 * @brief Global DSP and acquisition configuration constants.
 *
 * The constants in this file define buffer geometry, FIR processing granularity,
 * sampling frequency, and detector thresholds used by the entire signal chain.
 */

/** @brief Number of samples processed in one DSP frame per half-transfer. */
inline constexpr std::size_t BUFFER_SIZE = 1024U;
/** @brief FIR processing block size expected by CMSIS-DSP wrapper. */
inline constexpr std::size_t BLOCK_SIZE = 32U;
/** @brief Number of FIR taps in the envelope filter. */
inline constexpr std::size_t NUM_TAPS = 32U;
/** @brief Historical filter-order constant (kept for compatibility). */
inline constexpr std::size_t FIR_ORDER = 16U;
/** @brief Number of BLOCK_SIZE chunks inside one BUFFER_SIZE frame. */
inline constexpr std::size_t NUM_BLOCKS = BUFFER_SIZE / BLOCK_SIZE;

/** @brief Number of hydrophone/ADC channels processed in parallel. */
inline constexpr std::size_t ADC_CHANNELS = 4U;
/** @brief Envelope threshold for arrival detection. */
inline constexpr float SIGNAL_THRESHOLD = 1000.0f;
/** @brief ADC sampling frequency in hertz. */
inline constexpr float SAMPLE_RATE_HZ = 250000.0f;

/** @brief Samples available on each DMA half-transfer event. */
inline constexpr std::size_t DMA_HALF_BUFFER_SIZE = BUFFER_SIZE;
/** @brief Total circular DMA buffer size per channel (half + half). */
inline constexpr std::size_t DMA_FULL_BUFFER_SIZE = BUFFER_SIZE * 2U;
