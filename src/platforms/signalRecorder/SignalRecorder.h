#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "CommonSettings.h"
#include "stm32g4xx_hal.h"

namespace hydrv::hw {
class SignalRecorder
{
public:
    using SampleBuffer  = std::array<uint16_t, BUFFER_SIZE>;
    using SampleBuffers = std::array<SampleBuffer, ADC_CHANNELS>;
    bool startRecord() noexcept
    {
        bool ok = true;
        ok and HAL_TIM_Base_Stop(&_htim6);
        ok and (HAL_DAC_Start(&_hdac4, DAC_CHANNEL_1));
        ok and (HAL_DAC_SetValue(&_hdac4, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048));
        ok and (HAL_OPAMP_Start(&_hopamp4));

        ok and HAL_ADC_Start_DMA(&_hadc4, reinterpret_cast<uint32_t*>(_buffers[0].data()), SIGNAL_SAMPLE_BUFFER_SIZE);
        ok and HAL_ADC_Start_DMA(&_hadc5, reinterpret_cast<uint32_t*>(_buffers[1].data()), SIGNAL_SAMPLE_BUFFER_SIZE);
        ok and HAL_ADC_Start_DMA(&_hadc2, reinterpret_cast<uint32_t*>(_buffers[2].data()), SIGNAL_SAMPLE_BUFFER_SIZE);
        ok and HAL_ADC_Start_DMA(&_hadc1, reinterpret_cast<uint32_t*>(_buffers[3].data()), SIGNAL_SAMPLE_BUFFER_SIZE);
        ok and HAL_TIM_Base_Start(&_htim6);
        ok and HAL_TIM_Base_Start(&_htim7);
        return ok;
    }

    std::optional<SampleBuffers&> getSample()
    {
        if (_halfReady) {
            return
            {
                _buffers[0].data(), _buffers[1].data(), _buffers[2].data(), _buffers[3].data(),
            }
        }
        else if (_fullReady) {
            return
            {
                _buffers[0].data() + BUFFER_SIZE, _buffers[1].data() + BUFFER_SIZE, _buffers[2].data() + BUFFER_SIZE,
                    _buffers[3].data() + BUFFER_SIZE,
            }
        }
        else {
            return {};
        }
    }

    void halfReady(ADC_HandleTypeDef* hadc)
    {
        _halfReady = true;
        _fullReady = false;
    }

    void fullReady(ADC_HandleTypeDef* hadc)
    {
        _fullReady = true;
        _halfReady = false;
    }

    ADC_HandleTypeDef* adc1Handle()
    {
        return &_hadc1;
    }
    ADC_HandleTypeDef* adc2Handle()
    {
        return &_hadc2;
    }
    ADC_HandleTypeDef* adc4Handle()
    {
        return &_hadc4;
    }
    ADC_HandleTypeDef* adc5Handle()
    {
        return &_hadc5;
    }
    DAC_HandleTypeDef* dac4Handle()
    {
        return &_hdac4;
    }

    OPAMP_HandleTypeDef* opamp4Hanle()
    {
        return &_hopamp4;
    }

    TIM_HandleTypeDef* tim6Handle()
    {
        return &_htim6;
    }
    TIM_HandleTypeDef* tim7Handle()
    {
        return &_htim7;
    }

private:
    using SignalSampleBuffer  = std::array<uint16_t, SIGNAL_SAMPLE_BUFFER_SIZE>;
    using SignalSampleBuffers = std::array<SignalSampleBuffer, ADC_CHANNELS>;
    bool _fullReady           = false;
    bool _halfReady           = false;
    SignalSampleBuffers _buffers;
    DAC_HandleTypeDef _hdac4;
    TIM_HandleTypeDef _htim6;
    TIM_HandleTypeDef _htim7;
    OPAMP_HandleTypeDef _hopamp4;
    ADC_HandleTypeDef _hadc1;
    ADC_HandleTypeDef _hadc2;
    ADC_HandleTypeDef _hadc4;
    ADC_HandleTypeDef _hadc5;
};

} // namespace hydrv::hw
