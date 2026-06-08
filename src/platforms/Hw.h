#pragma once

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
#include "signalRecorder/SignalRecorder.h"
#include "telemetry/UartTelemetry.h"

namespace hydrv::hw {

uint32_t enterCriticalSection() noexcept;
void exitCriticalSection(uint32_t interruptState) noexcept;
void initialize(SignalRecorder& recorder, UartTelemetry& telemetry) noexcept;
[[noreturn]] void stop() noexcept;

extern "C" TIM_HandleTypeDef* GetHalTickTimer(void);

} // namespace hydrv::hw
#else
TIM_HandleTypeDef* GetHalTickTimer(void);
#endif
