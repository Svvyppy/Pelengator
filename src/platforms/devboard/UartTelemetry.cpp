#include "UartTelemetry.hpp"

#include <array>
#include <cmath>
#include <cstring>

#include "Hw.h"
#include "Pins.h"

namespace
{
constexpr std::size_t kQueueDepth = 8U;
constexpr std::size_t kMessageCapacity = 16U;

struct UartTelemetryQueue
{
    std::array<std::array<char, kMessageCapacity>, kQueueDepth> messages{};
    std::array<uint16_t, kQueueDepth> lengths{};

    std::size_t head = 0U;
    std::size_t count = 0U;
    bool tx_active = false;
};

UartTelemetryQueue g_queue{};

struct DelayTelemetryPacket
{
    int16_t d12_us = 0;
    int16_t d13_us = 0;
    int16_t d14_us = 0;
    float peleng_deg = 0.0f;
    float elevation_deg = 0.0f;
} __attribute__((packed));

static_assert(sizeof(DelayTelemetryPacket) == 14U);

std::size_t NextIndex(std::size_t index) { return (index + 1U) % kQueueDepth; }

bool IsTxReady(const UART_HandleTypeDef *huart) { return huart->gState == HAL_UART_STATE_READY; }

void SetRs485DirectionTx(void) { HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_SET); }

void SetRs485DirectionRx(void) { HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_RESET); }

bool IsInIsrContext(void) { return (__get_IPSR() != 0U); }

int16_t FloatMicrosecondsToI16(float value)
{
    constexpr float kMinInt16AsFloat = -32768.0f;
    constexpr float kMaxInt16AsFloat = 32767.0f;

    if (value <= kMinInt16AsFloat)
    {
        return -32768;
    }
    if (value >= kMaxInt16AsFloat)
    {
        return 32767;
    }

    return static_cast<int16_t>(std::lround(value));
}

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
    if (payload == nullptr || payload_length == 0U || payload_length > kMessageCapacity)
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
    if (!delays.valid || !delays.angles_valid)
    {
        return false;
    }

    const DelayTelemetryPacket packet{
        .d12_us = FloatMicrosecondsToI16(delays.d12_us),
        .d13_us = FloatMicrosecondsToI16(delays.d13_us),
        .d14_us = FloatMicrosecondsToI16(delays.d14_us),
        .peleng_deg = delays.peleng_deg,
        .elevation_deg = delays.elevation_deg,
    };

    const bool queued = EnqueueBytes(reinterpret_cast<const char *>(&packet), sizeof(packet));
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
    (void)message;
    return true;
}

EventLogger &EventLogger::operator<<(const char *message)
{
    (void)SendEventTelemetryUart(message);
    return *this;
}

EventLogger event{};

void SetPelengDebugSource(Peleng *peleng) { (void)peleng; }
