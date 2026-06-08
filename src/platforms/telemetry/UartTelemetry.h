#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "stm32g4xx_hal.h"

namespace hydrv::hw {

class UartTelemetry
{
public:
    bool write(std::string_view text) noexcept;
    void process() noexcept;

    UART_HandleTypeDef* uartHandle() noexcept
    {
        return &_uart;
    }

private:
    static constexpr std::size_t kQueueCapacity   = 8U;
    static constexpr std::size_t kMessageCapacity = 128U;

    struct Message
    {
        std::array<char, kMessageCapacity> text{};
        uint16_t length = 0U;
    };

    static uint32_t enterCriticalSection() noexcept;
    static void exitCriticalSection(uint32_t interruptState) noexcept;
    static void enableTransmitter() noexcept;
    static void disableTransmitter() noexcept;

    bool transmissionComplete() const noexcept;
    void removeTransmittedMessage() noexcept;
    void startNextTransmission() noexcept;

    std::array<Message, kQueueCapacity> _queuedMessages{};
    std::size_t _firstMessageIndex  = 0U;
    std::size_t _queuedMessageCount = 0U;
    bool _transmissionActive        = false;
    UART_HandleTypeDef _uart{};
};

} // namespace hydrv::hw
