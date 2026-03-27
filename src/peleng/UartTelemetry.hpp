#pragma once

#include "DelayEstimator.hpp"

/**
 * @brief Queue one delay telemetry frame for non-blocking UART transmit.
 * @return true when frame was queued, false when queue is full.
 */
bool SendDelayTelemetryUart(const DelayMeasurements &delays);
/**
 * @brief Progress UART TX state machine (call from main loop).
 */
void UartTelemetryProcess(void);
