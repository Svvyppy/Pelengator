#include "UartTelemetry.h"

#include <cstring>

#include "Pins.h"

namespace hydrv::hw {

uint32_t UartTelemetry::enterCriticalSection() noexcept
{
    const uint32_t interruptState = __get_PRIMASK();
    __disable_irq();
    return interruptState;
}

void UartTelemetry::exitCriticalSection(uint32_t interruptState) noexcept
{
    __set_PRIMASK(interruptState);
}

void UartTelemetry::enableTransmitter() noexcept
{
    HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_SET);
}

void UartTelemetry::disableTransmitter() noexcept
{
    HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_RESET);
}

bool UartTelemetry::write(std::string_view text) noexcept
{
    if (text.empty() || text.size() > kMessageCapacity) {
        return false;
    }

    const uint32_t interruptState = enterCriticalSection();
    if (_queuedMessageCount == kQueueCapacity) {
        exitCriticalSection(interruptState);
        return false;
    }

    const std::size_t messageIndex = (_firstMessageIndex + _queuedMessageCount) % kQueueCapacity;
    Message& message               = _queuedMessages[messageIndex];
    std::memcpy(message.text.data(), text.data(), text.size());
    message.length = static_cast<uint16_t>(text.size());
    ++_queuedMessageCount;
    exitCriticalSection(interruptState);
    return true;
}

void UartTelemetry::process() noexcept
{
    if (transmissionComplete()) {
        removeTransmittedMessage();
        disableTransmitter();
    }

    if (!_transmissionActive) {
        startNextTransmission();
    }
}

bool UartTelemetry::transmissionComplete() const noexcept
{
    return _transmissionActive && _uart.gState == HAL_UART_STATE_READY;
}

void UartTelemetry::removeTransmittedMessage() noexcept
{
    const uint32_t interruptState = enterCriticalSection();
    _firstMessageIndex            = (_firstMessageIndex + 1U) % kQueueCapacity;
    --_queuedMessageCount;
    _transmissionActive = false;
    exitCriticalSection(interruptState);
}

void UartTelemetry::startNextTransmission() noexcept
{
    const uint32_t interruptState = enterCriticalSection();
    if (_queuedMessageCount == 0U) {
        exitCriticalSection(interruptState);
        return;
    }

    Message& message    = _queuedMessages[_firstMessageIndex];
    _transmissionActive = true;
    exitCriticalSection(interruptState);

    enableTransmitter();
    if (HAL_UART_Transmit_IT(&_uart, reinterpret_cast<uint8_t*>(message.text.data()), message.length) != HAL_OK) {
        _transmissionActive = false;
        disableTransmitter();
    }
}

} // namespace hydrv::hw
