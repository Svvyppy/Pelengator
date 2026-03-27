#pragma once

#include "dsp/PelengDsp.hpp"

/**
 * @brief Queue one telemetry message for non-blocking UART transmission.
 * @param delays Delay frame to serialize.
 * @return false when queue is full or formatting failed.
 */
bool SendDelayTelemetryUart(const DelayMeasurements &delays);

/**
 * @brief Service UART telemetry state machine.
 *
 * Call periodically from the main loop to advance queued transmission.
 */
void UartTelemetryProcess(void);
