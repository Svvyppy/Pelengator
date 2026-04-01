#include "UartTelemetry.hpp"

#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "Hw.h"
#include "Peleng.hpp"
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
uint32_t g_event_drop_count = 0U;

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

bool IsInIsrContext(void) { return (__get_IPSR() != 0U); }

uint32_t EnterCritical(void)
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void ExitCritical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

bool EnqueueBytes(const char *payload, uint16_t payload_length)
{
    if (payload == nullptr || payload_length == 0U)
    {
        return false;
    }

    const uint32_t primask = EnterCritical();
    if (g_queue.count >= kQueueDepth)
    {
        ExitCritical(primask);
        return false;
    }

    const std::size_t slot = (g_queue.head + g_queue.count) % kQueueDepth;
    std::memcpy(g_queue.messages[slot].data(), payload, payload_length);
    g_queue.lengths[slot] = payload_length;
    ++g_queue.count;
    ExitCritical(primask);

    return true;
}

bool EnqueueFormattedMessage(const char *format, ...)
{
    char local_buffer[kMessageCapacity] = {};

    va_list args;
    va_start(args, format);
    const int formatted_length = std::vsnprintf(local_buffer, kMessageCapacity, format, args);
    va_end(args);

    const uint16_t payload_length = NormalizeLength(formatted_length);
    if (payload_length == 0U)
    {
        return false;
    }

    return EnqueueBytes(local_buffer, payload_length);
}

bool EnqueueEventMessage(const char *message)
{
    if (message == nullptr)
    {
        return false;
    }

    char local_buffer[kMessageCapacity] = {};
    uint16_t len = 0U;

    constexpr char kPrefix[] = "EVT ";
    for (std::size_t i = 0U; i < sizeof(kPrefix) - 1U; ++i)
    {
        local_buffer[len++] = kPrefix[i];
    }

    while (*message != '\0' && len < static_cast<uint16_t>(kMessageCapacity - 3U))
    {
        local_buffer[len++] = *message++;
    }

    local_buffer[len++] = '\r';
    local_buffer[len++] = '\n';

    return EnqueueBytes(local_buffer, len);
}

void QueueAliveMessageIfDue(void)
{
    const uint32_t now = HAL_GetTick();
    if ((now - g_last_alive_tick) < kAlivePeriodMs)
    {
        return;
    }

    uint32_t drops = 0U;
    std::size_t queued = 0U;
    const uint32_t dma_half = PelengGetDmaHalfCount();
    const uint32_t dma_full = PelengGetDmaFullCount();
    {
        const uint32_t primask = EnterCritical();
        drops = g_event_drop_count;
        queued = g_queue.count;
        ExitCritical(primask);
    }

    if (EnqueueFormattedMessage("ALIVE t=%lu RS485=TX q=%lu drop=%lu dmaH=%lu dmaF=%lu\\r\\n",
                                static_cast<unsigned long>(now), static_cast<unsigned long>(queued),
                                static_cast<unsigned long>(drops), static_cast<unsigned long>(dma_half),
                                static_cast<unsigned long>(dma_full)))
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

    bool tx_completed = false;
    {
        const uint32_t primask = EnterCritical();
        if (g_queue.tx_active && IsTxReady(huart))
        {
            g_queue.head = NextIndex(g_queue.head);
            --g_queue.count;
            g_queue.tx_active = false;
            tx_completed = true;
        }
        ExitCritical(primask);
    }

    if (tx_completed)
    {
        SetRs485DirectionRx();
    }

    QueueAliveMessageIfDue();

    if (!IsTxReady(huart))
    {
        return;
    }

    const char *tx_ptr = nullptr;
    uint16_t tx_len = 0U;
    {
        const uint32_t primask = EnterCritical();
        if (!g_queue.tx_active && g_queue.count > 0U)
        {
            tx_ptr = g_queue.messages[g_queue.head].data();
            tx_len = g_queue.lengths[g_queue.head];
            g_queue.tx_active = true;
        }
        ExitCritical(primask);
    }

    if (tx_ptr == nullptr || tx_len == 0U)
    {
        return;
    }

    SetRs485DirectionTx();
    const HAL_StatusTypeDef tx_status = HAL_UART_Transmit_IT(huart, reinterpret_cast<uint8_t *>(const_cast<char *>(tx_ptr)),
                                                             tx_len);

    if (tx_status == HAL_OK)
    {
        return;
    }

    {
        const uint32_t primask = EnterCritical();
        g_queue.tx_active = false;
        ExitCritical(primask);
    }

    if (tx_status != HAL_OK)
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

    if (!IsInIsrContext())
    {
        UartTelemetryProcess();
    }
    return true;
}

bool SendEventTelemetryUart(const char *message)
{
    const bool queued = EnqueueEventMessage(message);
    if (!queued)
    {
        const uint32_t primask = EnterCritical();
        ++g_event_drop_count;
        ExitCritical(primask);
        return false;
    }

    if (!IsInIsrContext())
    {
        UartTelemetryProcess();
    }
    return true;
}

EventLogger &EventLogger::operator<<(const char *message)
{
    (void)SendEventTelemetryUart(message);
    return *this;
}

EventLogger event{};
