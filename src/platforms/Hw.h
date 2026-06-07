#pragma once

#include <stdbool.h>

#include "signalRecorder/SignalRecorder.h"

namespace hydrv::hw {
/**
 * @brief Bundle of initialized STM32 HAL peripheral handles.
 */
struct HwInstances
{
    SignalRecorder* recorder;

    TIM_HandleTypeDef htim15;
};

/**
 * @brief Return pointer to global hardware handles.
 */
struct HwInstances* GetHwInstances(void);

/**
 * @brief Initialize board peripherals required by the firmware.
 * @return true on success.
 */
bool InitHw(void);

} // namespace hydrv::hw
