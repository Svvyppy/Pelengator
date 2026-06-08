#include "SignalRecorder.h"

#include <cassert>

namespace hydrv::hw {

bool SignalRecorder::BlockAwaiter::await_ready() const noexcept
{
    return _recorder.hasReadyBlock();
}

bool SignalRecorder::BlockAwaiter::await_suspend(std::coroutine_handle<> awaiting) noexcept
{
    return _recorder.waitForBlock(awaiting);
}

SignalRecorder::RecordedBlock SignalRecorder::BlockAwaiter::await_resume() noexcept
{
    return _recorder.takeReadyBlock();
}

uint32_t SignalRecorder::enterCriticalSection() noexcept
{
    const uint32_t interruptState = __get_PRIMASK();
    __disable_irq();
    return interruptState;
}

void SignalRecorder::exitCriticalSection(uint32_t interruptState) noexcept
{
    __set_PRIMASK(interruptState);
}

bool SignalRecorder::startRecording() noexcept
{
    stopRecording();

    for (DmaBuffer& buffer : _dmaBuffers) {
        buffer.fill(0U);
    }
    _waitingCoroutine           = nullptr;
    _completedFirstHalfAdcMask  = 0U;
    _completedSecondHalfAdcMask = 0U;
    _readyBlock                 = ReadyBlock::None;

    if (HAL_DAC_Start(&_dac4, DAC_CHANNEL_1) != HAL_OK ||
        HAL_DAC_SetValue(&_dac4, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048U) != HAL_OK ||
        HAL_OPAMP_Start(&_opamp4) != HAL_OK || !startAdcDma(_adc4, _dmaBuffers[0]) ||
        !startAdcDma(_adc5, _dmaBuffers[1]) || !startAdcDma(_adc2, _dmaBuffers[2]) ||
        !startAdcDma(_adc1, _dmaBuffers[3]) || HAL_TIM_Base_Start(&_samplingTimer) != HAL_OK ||
        HAL_TIM_Base_Start(&_serviceTimer) != HAL_OK)
    {
        stopRecording();
        return false;
    }

    return true;
}

bool SignalRecorder::hasReadyBlock() const noexcept
{
    const uint32_t interruptState = enterCriticalSection();
    const bool ready              = _readyBlock != ReadyBlock::None;
    exitCriticalSection(interruptState);
    return ready;
}

bool SignalRecorder::waitForBlock(std::coroutine_handle<> awaiting) noexcept
{
    const uint32_t interruptState = enterCriticalSection();
    if (_readyBlock != ReadyBlock::None) {
        exitCriticalSection(interruptState);
        return false;
    }

    assert(!_waitingCoroutine);
    _waitingCoroutine = awaiting;
    exitCriticalSection(interruptState);
    return true;
}

SignalRecorder::RecordedBlock SignalRecorder::takeReadyBlock() noexcept
{
    const uint32_t interruptState = enterCriticalSection();
    const ReadyBlock readyBlock   = _readyBlock;
    _readyBlock                   = ReadyBlock::None;
    exitCriticalSection(interruptState);

    assert(readyBlock != ReadyBlock::None);
    return makeBlock((readyBlock == ReadyBlock::FirstHalf) ? 0U : kSignalBlockSize);
}

void SignalRecorder::onHalfTransferComplete(ADC_HandleTypeDef* completedAdc) noexcept
{
    markAdcComplete(completedAdc, _completedFirstHalfAdcMask, ReadyBlock::FirstHalf);
}

void SignalRecorder::onTransferComplete(ADC_HandleTypeDef* completedAdc) noexcept
{
    markAdcComplete(completedAdc, _completedSecondHalfAdcMask, ReadyBlock::SecondHalf);
}

bool SignalRecorder::startAdcDma(ADC_HandleTypeDef& adc, DmaBuffer& destination) noexcept
{
    return HAL_ADC_Start_DMA(&adc, reinterpret_cast<uint32_t*>(destination.data()), destination.size()) == HAL_OK;
}

void SignalRecorder::stopRecording() noexcept
{
    (void)HAL_TIM_Base_Stop(&_samplingTimer);
    (void)HAL_TIM_Base_Stop(&_serviceTimer);
    (void)HAL_ADC_Stop_DMA(&_adc1);
    (void)HAL_ADC_Stop_DMA(&_adc2);
    (void)HAL_ADC_Stop_DMA(&_adc4);
    (void)HAL_ADC_Stop_DMA(&_adc5);
    (void)HAL_OPAMP_Stop(&_opamp4);
    (void)HAL_DAC_Stop(&_dac4, DAC_CHANNEL_1);
}

uint8_t SignalRecorder::adcCompletionBit(const ADC_HandleTypeDef* completedAdc) const noexcept
{
    if (completedAdc == &_adc4) {
        return 1U << 0U;
    }
    if (completedAdc == &_adc5) {
        return 1U << 1U;
    }
    if (completedAdc == &_adc2) {
        return 1U << 2U;
    }
    if (completedAdc == &_adc1) {
        return 1U << 3U;
    }
    return 0U;
}

void SignalRecorder::markAdcComplete(ADC_HandleTypeDef* completedAdc,
                                     volatile uint8_t& completedAdcMask,
                                     ReadyBlock completedBlock) noexcept
{
    completedAdcMask = completedAdcMask | adcCompletionBit(completedAdc);
    if (completedAdcMask != kAllAdcsCompleted) {
        return;
    }

    completedAdcMask = 0U;
    _readyBlock      = completedBlock;
    if (_waitingCoroutine && _scheduler.schedule(_waitingCoroutine)) {
        _waitingCoroutine = nullptr;
    }
}

SignalRecorder::RecordedBlock SignalRecorder::makeBlock(std::size_t sampleOffset) const noexcept
{
    return RecordedBlock{
        .channelSamples =
            {
                ChannelSamples{_dmaBuffers[0].data() + sampleOffset, kSignalBlockSize},
                ChannelSamples{_dmaBuffers[1].data() + sampleOffset, kSignalBlockSize},
                ChannelSamples{_dmaBuffers[2].data() + sampleOffset, kSignalBlockSize},
                ChannelSamples{_dmaBuffers[3].data() + sampleOffset, kSignalBlockSize},
            },
    };
}

} // namespace hydrv::hw
