#include "UartTelemetry.hpp"

#include <array>
#include <cstdarg>
#include <cstdio>

#include "Hw.h"
#include "Pins.h"

namespace
{
constexpr std::size_t kQueueDepth = 8U;
constexpr std::size_t kMessageCapacity = 128U;
constexpr uint32_t kAlivePeriodMs = 1000U;

struct UartTelemetryQueue
{
    std::array<std::array<char, kMessageCapacity>, kQueueDepth> messages{};
    std::array<uint16_t, kQueueDepth> lengths{};

    std::size_t head = 0U;
    std::size_t count = 0U;
    bool tx_active = false;
};

UartTelemetryQueue g_queue{};
uint32_t g_last_alive_tick = 0U;

std::size_t NextIndex(std::size_t index) { return (index + 1U) % kQueueDepth; }

uint16_t NormalizeLength(int formatted_length)
{
    if (formatted_length <= 0)
    {
        return 0U;
    }

    if (formatted_length >= static_cast<int>(kMessageCapacity))
    {
        return static_cast<uint16_t>(kMessageCapacity - 1U);
    }

    return static_cast<uint16_t>(formatted_length);
}

bool IsTxReady(const UART_HandleTypeDef *huart) { return huart->gState == HAL_UART_STATE_READY; }

void SetRs485DirectionTx(void) { HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_SET); }

void SetRs485DirectionRx(void) { HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_RESET); }

bool EnqueueFormattedMessage(const char *format, ...)
{
    if (g_queue.count >= kQueueDepth)
    {
        return false;
    }

    const std::size_t slot = (g_queue.head + g_queue.count) % kQueueDepth;
    char *const tx_buffer = g_queue.messages[slot].data();

    va_list args;
    va_start(args, format);
    const int formatted_length = std::vsnprintf(tx_buffer, kMessageCapacity, format, args);
    va_end(args);

    const uint16_t payload_length = NormalizeLength(formatted_length);
    if (payload_length == 0U)
    {
        return false;
    }

    g_queue.lengths[slot] = payload_length;
    ++g_queue.count;
    return true;
}

void QueueAliveMessageIfDue(void)
{
    const uint32_t now = HAL_GetTick();
    if ((now - g_last_alive_tick) < kAlivePeriodMs)
    {
        return;
    }

    if (EnqueueFormattedMessage("ALIVE t=%lu RS485=TX\\r\\n", static_cast<unsigned long>(now)))
    {
        g_last_alive_tick = now;
    }
}
} // namespace

/**
 * @brief Advance UART telemetry queue and start/continue async TX.
 *
 * This function is intentionally non-blocking and should be called from the main loop.
 */
void UartTelemetryProcess(void)
{
    UART_HandleTypeDef *huart = &GetHwInstances()->huart1;

    if (g_queue.tx_active)
    {
        if (!IsTxReady(huart))
        {
            return;
        }

        g_queue.head = NextIndex(g_queue.head);
        --g_queue.count;
        g_queue.tx_active = false;
        SetRs485DirectionRx();
    }

    QueueAliveMessageIfDue();

    if (g_queue.count == 0U || !IsTxReady(huart))
    {
        return;
    }

    SetRs485DirectionTx();
    const HAL_StatusTypeDef tx_status = HAL_UART_Transmit_IT(
        huart, reinterpret_cast<uint8_t *>(g_queue.messages[g_queue.head].data()), g_queue.lengths[g_queue.head]);

    if (tx_status == HAL_OK)
    {
        g_queue.tx_active = true;
    }
    else
    {
        SetRs485DirectionRx();
    }
}

/**
 * @brief Serialize one delay frame into queue and trigger async transmit.
 */
bool SendDelayTelemetryUart(const DelayMeasurements &delays)
{
    bool queued = false;
    if (delays.valid)
    {
        queued = EnqueueFormattedMessage("RS485=TX D12=%ld(%0.2fus) D13=%ld(%0.2fus) D14=%ld(%0.2fus)\\r\\n",
                                         static_cast<long>(delays.d12_samples), static_cast<double>(delays.d12_us),
                                         static_cast<long>(delays.d13_samples), static_cast<double>(delays.d13_us),
                                         static_cast<long>(delays.d14_samples), static_cast<double>(delays.d14_us));
    }
    else
    {
        queued = EnqueueFormattedMessage("RS485=TX NO_SIGNAL\\r\\n");
    }

    if (!queued)
    {
        return false;
    }

    UartTelemetryProcess();
    return true;
}

bool SendEventTelemetryUart(const char *message)
{
    if (message == nullptr)
    {
        return false;
    }

    const bool queued = EnqueueFormattedMessage("EVT %s\\r\\n", message);
    if (!queued)
    {
        return false;
    }

    UartTelemetryProcess();
    return true;
}

EventLogger &EventLogger::operator<<(const char *message)
{
    (void)SendEventTelemetryUart(message);
    return *this;
}

EventLogger event{};
