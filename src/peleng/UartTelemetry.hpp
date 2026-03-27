#pragma once

#include "DelayEstimator.hpp"

bool SendDelayTelemetryUart(const DelayMeasurements &delays);
void UartTelemetryProcess(void);
