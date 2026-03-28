#pragma once

#include "DelayEstimator.hpp"

/**
 * @brief Queue one delay telemetry frame for non-blocking UART transmit.
 * @return true when frame was queued, false when queue is full.
 */
bool SendDelayTelemetryUart(const DelayMeasurements &delays);

/**
 * @brief Queue one textual event line for non-blocking UART transmit.
 * @return true when event was queued, false when queue is full.
 */
bool SendEventTelemetryUart(const char *message);
/**
 * @brief Progress UART TX state machine (call from main loop).
 */
void UartTelemetryProcess(void);

class EventLogger
{
public:
    EventLogger &operator<<(const char *message);
};

extern EventLogger event;
