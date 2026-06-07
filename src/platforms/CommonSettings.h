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
/** @brief Lower envelope threshold for arrival detection in Q15 units. */
static inline constexpr const int16_t SIGNAL_THRESHOLD_Q15 = 700;
/** @brief Upper envelope threshold; larger values are treated as signal body, not front. */
static inline constexpr const int16_t SIGNAL_THRESHOLD_MAX_Q15 = 1000;
/** @brief ADC sampling frequency in hertz. */
static inline constexpr const float SAMPLE_RATE_HZ = 250000.0f;
/** @brief Assumed speed of sound in water in meters per second. */
static inline constexpr const float SOUND_SPEED_MPS = 1441.0f;
/** @brief Extra tolerance for physically possible inter-channel delay checks. */
static inline constexpr const int32_t SIGNAL_DELAY_MARGIN_SAMPLES = 16;

/** @brief Samples processed when one acquisition block is ready. */
static inline constexpr const std::size_t SIGNAL_BLOCK_SIZE = BUFFER_SIZE;
/** @brief Samples kept by the acquisition backend per channel. */
static inline constexpr const std::size_t SIGNAL_SAMPLE_BUFFER_SIZE = BUFFER_SIZE * 2U;

/** @brief Hydrophone coordinates in meters, indexed by logical channel. */
static inline constexpr const float HYDROPHONE_POSITIONS_M[ADC_CHANNELS][3] = {
    { 0.0625f,  0.1175f,   0.0f },
    { 0.0625f, -0.1175f,   0.0f },
    {    0.0f,  -0.037f, 0.220f },
    { -0.295f,     0.0f,   0.0f },
};
