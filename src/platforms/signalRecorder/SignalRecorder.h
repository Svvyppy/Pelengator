#pragma once

#include <array>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <span>

#include "CommonSettings.h"
#include "Scheduler.h"
#include "stm32g4xx_hal.h"

namespace hydrv::hw {

class SignalRecorder
{
public:
    using ChannelSamples = std::span<const uint16_t, kSignalBlockSize>;

    struct RecordedBlock
    {
        std::array<ChannelSamples, kAdcChannelCount> channelSamples;
    };

    class BlockAwaiter
    {
    public:
        explicit BlockAwaiter(SignalRecorder& recorder) noexcept
            : _recorder{ recorder }
        {}

        bool await_ready() const noexcept;
        bool await_suspend(std::coroutine_handle<> awaiting) noexcept;
        RecordedBlock await_resume() noexcept;

    private:
        SignalRecorder& _recorder;
    };

    explicit SignalRecorder(async::Scheduler& scheduler) noexcept
        : _scheduler{ scheduler }
    {}

    bool startRecording() noexcept;
    BlockAwaiter nextBlock() noexcept
    {
        return BlockAwaiter{ *this };
    }

    void onHalfTransferComplete(ADC_HandleTypeDef* completedAdc) noexcept;
    void onTransferComplete(ADC_HandleTypeDef* completedAdc) noexcept;

    ADC_HandleTypeDef* adc1Handle() noexcept
    {
        return &_adc1;
    }
    ADC_HandleTypeDef* adc2Handle() noexcept
    {
        return &_adc2;
    }
    ADC_HandleTypeDef* adc4Handle() noexcept
    {
        return &_adc4;
    }
    ADC_HandleTypeDef* adc5Handle() noexcept
    {
        return &_adc5;
    }
    DAC_HandleTypeDef* dac4Handle() noexcept
    {
        return &_dac4;
    }
    OPAMP_HandleTypeDef* opamp4Handle() noexcept
    {
        return &_opamp4;
    }
    TIM_HandleTypeDef* samplingTimerHandle() noexcept
    {
        return &_samplingTimer;
    }
    TIM_HandleTypeDef* serviceTimerHandle() noexcept
    {
        return &_serviceTimer;
    }

private:
    enum class ReadyBlock : uint8_t
    {
        None,
        FirstHalf,
        SecondHalf,
    };

    static constexpr uint8_t kAllAdcsCompleted = (1U << kAdcChannelCount) - 1U;
    using DmaBuffer                            = std::array<uint16_t, kDmaBufferSize>;

    static uint32_t enterCriticalSection() noexcept;
    static void exitCriticalSection(uint32_t interruptState) noexcept;
    bool hasReadyBlock() const noexcept;
    bool waitForBlock(std::coroutine_handle<> awaiting) noexcept;
    RecordedBlock takeReadyBlock() noexcept;
    bool startAdcDma(ADC_HandleTypeDef& adc, DmaBuffer& destination) noexcept;
    void stopRecording() noexcept;
    uint8_t adcCompletionBit(const ADC_HandleTypeDef* completedAdc) const noexcept;
    void markAdcComplete(ADC_HandleTypeDef* completedAdc,
                         volatile uint8_t& completedAdcMask,
                         ReadyBlock completedBlock) noexcept;
    RecordedBlock makeBlock(std::size_t sampleOffset) const noexcept;

    async::Scheduler& _scheduler;
    std::array<DmaBuffer, kAdcChannelCount> _dmaBuffers{};
    std::coroutine_handle<> _waitingCoroutine    = nullptr;
    volatile uint8_t _completedFirstHalfAdcMask  = 0U;
    volatile uint8_t _completedSecondHalfAdcMask = 0U;
    volatile ReadyBlock _readyBlock              = ReadyBlock::None;

    DAC_HandleTypeDef _dac4{};
    OPAMP_HandleTypeDef _opamp4{};
    TIM_HandleTypeDef _samplingTimer{};
    TIM_HandleTypeDef _serviceTimer{};
    ADC_HandleTypeDef _adc1{};
    ADC_HandleTypeDef _adc2{};
    ADC_HandleTypeDef _adc4{};
    ADC_HandleTypeDef _adc5{};
};

} // namespace hydrv::hw
