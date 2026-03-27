#pragma once

#include <array>
#include <cstdint>
#include <cstring>

#ifdef __cplusplus
extern "C"
{
#endif

#include <CommonSettings.h>

#include "arm_math.h"
#include <Filter.hpp>

#include "Hw.h"

#ifdef __cplusplus
}
#endif

class Peleng
{
public:
    Peleng()
    {
        adc1_buffer.fill(0);
        adc2_buffer.fill(0);
        adc3_buffer.fill(0);
        adc4_buffer.fill(0);

        work_buffer1.fill(0);
        work_buffer2.fill(0);
        work_buffer3.fill(0);
        work_buffer4.fill(0);

        squared_buffer.fill(0);
    }

    ~Peleng() = default;

    inline void Init()
    {
        InitHw();
        InitAdcs();
    }

    inline void Process()
    {
        if (adc_half_flag)
        {
            envelope_buffer1 = ProcessAdcChannel(work_buffer1, fir1, 0);
            envelope_buffer2 = ProcessAdcChannel(work_buffer2, fir1, 0);
            envelope_buffer3 = ProcessAdcChannel(work_buffer3, fir1, 0);
            envelope_buffer4 = ProcessAdcChannel(work_buffer4, fir1, 0);
            adc_half_flag = 0;
        }
        if (adc_full_flag)
        {
            envelope_buffer1 = ProcessAdcChannel(work_buffer1, fir1, BUFFER_SIZE);
            envelope_buffer2 = ProcessAdcChannel(work_buffer2, fir1, BUFFER_SIZE);
            envelope_buffer3 = ProcessAdcChannel(work_buffer3, fir1, BUFFER_SIZE);
            envelope_buffer4 = ProcessAdcChannel(work_buffer4, fir1, BUFFER_SIZE);
            adc_full_flag = 0;
        }
    }

    void DmaHalfTransferCallback(ADC_HandleTypeDef *hadc)
    {
        ConvertAdcToF32(adc1_buffer.data(), work_buffer1.data(), BUFFER_SIZE);
        ConvertAdcToF32(adc2_buffer.data(), work_buffer2.data(), BUFFER_SIZE);
        ConvertAdcToF32(adc3_buffer.data(), work_buffer3.data(), BUFFER_SIZE);
        ConvertAdcToF32(adc4_buffer.data(), work_buffer4.data(), BUFFER_SIZE);
        adc_half_flag = 1;
    }

    void DmaTransferCompleteCallback(ADC_HandleTypeDef *hadc)
    {
        ConvertAdcToF32(adc1_buffer.data() + BUFFER_SIZE, work_buffer1.data() + BUFFER_SIZE, BUFFER_SIZE);
        ConvertAdcToF32(adc2_buffer.data() + BUFFER_SIZE, work_buffer2.data() + BUFFER_SIZE, BUFFER_SIZE);
        ConvertAdcToF32(adc3_buffer.data() + BUFFER_SIZE, work_buffer3.data() + BUFFER_SIZE, BUFFER_SIZE);
        ConvertAdcToF32(adc4_buffer.data() + BUFFER_SIZE, work_buffer4.data() + BUFFER_SIZE, BUFFER_SIZE);
        adc_full_flag = 0;
    }

private:
    std::array<uint32_t, BUFFER_SIZE * 2> adc1_buffer;
    std::array<uint32_t, BUFFER_SIZE * 2> adc2_buffer;
    std::array<uint32_t, BUFFER_SIZE * 2> adc3_buffer;
    std::array<uint32_t, BUFFER_SIZE * 2> adc4_buffer;

    std::array<float32_t, BUFFER_SIZE> work_buffer1;
    std::array<float32_t, BUFFER_SIZE> work_buffer2;
    std::array<float32_t, BUFFER_SIZE> work_buffer3;
    std::array<float32_t, BUFFER_SIZE> work_buffer4;

    std::array<float32_t, BUFFER_SIZE> envelope_buffer1;
    std::array<float32_t, BUFFER_SIZE> envelope_buffer2;
    std::array<float32_t, BUFFER_SIZE> envelope_buffer3;
    std::array<float32_t, BUFFER_SIZE> envelope_buffer4;

    std::array<float32_t, BUFFER_SIZE> squared_buffer;

    Filter fir1{};

    volatile bool adc_half_flag = 0;
    volatile bool adc_full_flag = 0;

    inline void InitAdcs()
    {
        HAL_TIM_Base_Stop(&GetHwInstances()->htim6);
        HAL_ADC_Start_DMA(&GetHwInstances()->hadc1, (uint32_t *)adc1_buffer.data(), BUFFER_SIZE * 2);
        HAL_ADC_Start_DMA(&GetHwInstances()->hadc2, (uint32_t *)adc2_buffer.data(), BUFFER_SIZE * 2);
        HAL_ADC_Start_DMA(&GetHwInstances()->hadc3, (uint32_t *)adc3_buffer.data(), BUFFER_SIZE * 2);
        HAL_ADC_Start_DMA(&GetHwInstances()->hadc5, (uint32_t *)adc4_buffer.data(), BUFFER_SIZE * 2);
    }

    inline void ConvertAdcToF32(const uint32_t *source, float32_t *destination, uint32_t length)
    {
        for (uint32_t i = 0; i < length; ++i)
        {
            int32_t raw = static_cast<int32_t>(source[i] & 0xFFF);

            int32_t centered = raw - 2048;

            int64_t scaled = static_cast<int64_t>(centered) * (1 << 19);

            if (scaled > static_cast<int64_t>(INT32_MAX))
            {
                destination[i] = static_cast<float32_t>(INT32_MAX);
            }
            else if (scaled < static_cast<int64_t>(INT32_MIN))
            {
                destination[i] = static_cast<float32_t>(INT32_MIN);
            }
            else
            {
                destination[i] = static_cast<float32_t>(scaled);
            }
        }
    }

    inline void SquareBuffer(const float32_t *input, float32_t *output, uint32_t length)
    {
        arm_mult_f32(input, input, output, length);
    }

    inline std::array<float32_t, BUFFER_SIZE> ProcessAdcChannel(std::array<float32_t, BUFFER_SIZE> &input, Filter &fir,
                                                                uint32_t start_index)
    {
        SquareBuffer(input.data(), squared_buffer.data(), BUFFER_SIZE);
        return fir.applyFilter(squared_buffer);
    }
};
