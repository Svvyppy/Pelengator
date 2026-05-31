#include "UartTelemetry.hpp"

#include <array>
#include <cstdint>
#include <cstring>

#include "Hw.h"

#include "Pins.h"

namespace {
constexpr std::size_t kQueueDepth      = 8U;
constexpr std::size_t kMessageCapacity = 128U;

struct UartTelemetryQueue
{
    std::array<std::array<char, kMessageCapacity>, kQueueDepth> messages{};
    std::array<uint16_t, kQueueDepth> lengths{};

    std::size_t head  = 0U;
    std::size_t count = 0U;
    bool tx_active    = false;
};

struct FixedTenths
{
    const char* sign  = "";
    uint32_t whole    = 0;
    uint32_t fraction = 0;
};

struct LineBuffer
{
    std::array<char, kMessageCapacity> text{};
    std::size_t length = 0U;
    bool valid         = true;
};

UartTelemetryQueue g_queue{};

std::size_t NextIndex(std::size_t index)
{
    return (index + 1U) % kQueueDepth;
}

bool IsTxReady(const UART_HandleTypeDef* huart)
{
    return huart->gState == HAL_UART_STATE_READY;
}

void SetRs485DirectionTx(void)
{
    HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_SET);
}

void SetRs485DirectionRx(void)
{
    HAL_GPIO_WritePin(TX_En_GPIO_Port, TX_En_Pin, GPIO_PIN_RESET);
}

bool IsInIsrContext(void)
{
    return (__get_IPSR() != 0U);
}

int32_t RoundFloatToInt32(float value)
{
    constexpr float kMinInt32AsFloat = -2147483000.0f;
    constexpr float kMaxInt32AsFloat = 2147483000.0f;

    if (value <= kMinInt32AsFloat) {
        return -2147483000;
    }
    if (value >= kMaxInt32AsFloat) {
        return 2147483000;
    }

    return static_cast<int32_t>((value >= 0.0f) ? (value + 0.5f) : (value - 0.5f));
}

uint32_t MagnitudeInt32(int32_t value)
{
    if (value >= 0) {
        return static_cast<uint32_t>(value);
    }

    return static_cast<uint32_t>(-(value + 1)) + 1U;
}

FixedTenths MakeFixedTenths(float value)
{
    const int32_t tenths     = RoundFloatToInt32(value * 10.0f);
    const uint32_t magnitude = MagnitudeInt32(tenths);

    return FixedTenths{
        .sign     = (tenths < 0) ? "-" : "",
        .whole    = magnitude / 10,
        .fraction = magnitude % 10,
    };
}

bool AppendChar(LineBuffer& line, char value)
{
    if (!line.valid || line.length >= line.text.size()) {
        line.valid = false;
        return false;
    }

    line.text[line.length] = value;
    ++line.length;
    return true;
}

bool AppendString(LineBuffer& line, const char* value)
{
    if (value == nullptr) {
        line.valid = false;
        return false;
    }

    while (*value != '\0' && line.valid) {
        (void)AppendChar(line, *value);
        ++value;
    }

    return line.valid;
}

bool AppendUnsigned(LineBuffer& line, uint32_t value)
{
    std::array<char, 10U> digits{};
    std::size_t digit_count = 0U;

    do {
        digits[digit_count]  = static_cast<char>('0' + (value % 10U));
        value               /= 10U;
        ++digit_count;
    } while (value != 0U && digit_count < digits.size());

    while (digit_count > 0U && line.valid) {
        --digit_count;
        (void)AppendChar(line, digits[digit_count]);
    }

    return line.valid;
}

bool AppendSigned(LineBuffer& line, int32_t value)
{
    if (value < 0) {
        (void)AppendChar(line, '-');
    }

    return AppendUnsigned(line, MagnitudeInt32(value));
}

bool AppendFixedTenths(LineBuffer& line, const FixedTenths& value)
{
    return AppendString(line, value.sign) && AppendUnsigned(line, value.whole) && AppendChar(line, '.') &&
           AppendChar(line, static_cast<char>('0' + value.fraction));
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

bool EnqueueBytes(const char* payload, uint16_t payload_length)
{
    if (payload == nullptr || payload_length == 0U || payload_length > kMessageCapacity) {
        return false;
    }

    const uint32_t primask = EnterCritical();
    if (g_queue.count >= kQueueDepth) {
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

bool EnqueueLine(const LineBuffer& line)
{
    if (!line.valid || line.length == 0U) {
        return false;
    }

    return EnqueueBytes(line.text.data(), static_cast<uint16_t>(line.length));
}

} // namespace

/**
 * @brief Advance UART telemetry queue and start/continue async TX.
 *
 * This function is intentionally non-blocking and should be called from the main loop.
 */
void UartTelemetryProcess(void)
{
    UART_HandleTypeDef* huart = &GetHwInstances()->huart1;

    bool tx_completed = false;
    {
        const uint32_t primask = EnterCritical();
        if (g_queue.tx_active && IsTxReady(huart)) {
            g_queue.head = NextIndex(g_queue.head);
            --g_queue.count;
            g_queue.tx_active = false;
            tx_completed      = true;
        }
        ExitCritical(primask);
    }

    if (tx_completed) {
        SetRs485DirectionRx();
    }

    if (!IsTxReady(huart)) {
        return;
    }

    const char* tx_ptr = nullptr;
    uint16_t tx_len    = 0U;
    {
        const uint32_t primask = EnterCritical();
        if (!g_queue.tx_active && g_queue.count > 0U) {
            tx_ptr            = g_queue.messages[g_queue.head].data();
            tx_len            = g_queue.lengths[g_queue.head];
            g_queue.tx_active = true;
        }
        ExitCritical(primask);
    }

    if (tx_ptr == nullptr || tx_len == 0U) {
        return;
    }

    SetRs485DirectionTx();
    const HAL_StatusTypeDef tx_status =
        HAL_UART_Transmit_IT(huart, reinterpret_cast<uint8_t*>(const_cast<char*>(tx_ptr)), tx_len);

    if (tx_status == HAL_OK) {
        return;
    }

    {
        const uint32_t primask = EnterCritical();
        g_queue.tx_active      = false;
        ExitCritical(primask);
    }

    if (tx_status != HAL_OK) {
        SetRs485DirectionRx();
    }
}

/**
 * @brief Serialize one delay frame into queue and trigger async transmit.
 */
bool SendDelayTelemetryUart(const DelayMeasurements& delays)
{
    if (!delays.valid || !delays.angles_valid) {
        // constexpr char kNoSignalMessage[] = "NO_SIGNAL\r\n";
        // const bool queued = EnqueueBytes(kNoSignalMessage, sizeof(kNoSignalMessage) - 1U);
        // if (queued && !IsInIsrContext())
        // {
        //     UartTelemetryProcess();
        // }
        return true;
    }

    const FixedTenths peleng    = MakeFixedTenths(delays.peleng_deg);
    const FixedTenths elevation = MakeFixedTenths(delays.elevation_deg);

    LineBuffer line{};
    (void)AppendString(line, "D12=");
    (void)AppendSigned(line, RoundFloatToInt32(delays.d12_us));
    (void)AppendString(line, "us D13=");
    (void)AppendSigned(line, RoundFloatToInt32(delays.d13_us));
    (void)AppendString(line, "us D14=");
    (void)AppendSigned(line, RoundFloatToInt32(delays.d14_us));
    (void)AppendString(line, "us P=");
    (void)AppendFixedTenths(line, peleng);
    (void)AppendString(line, "deg E=");
    (void)AppendFixedTenths(line, elevation);
    (void)AppendString(line, "deg\r\n");

    const bool queued = EnqueueLine(line);
    if (!queued) {
        return false;
    }

    if (!IsInIsrContext()) {
        UartTelemetryProcess();
    }
    return true;
}

bool SendEventTelemetryUart(const char* message)
{
    if (message == nullptr) {
        return false;
    }

    LineBuffer event_message{};
    (void)AppendString(event_message, "EV ");
    (void)AppendString(event_message, message);
    (void)AppendString(event_message, "\r\n");

    const bool queued = EnqueueLine(event_message);
    if (queued && !IsInIsrContext()) {
        UartTelemetryProcess();
    }
    return queued;
}

EventLogger& EventLogger::operator<<(const char* message)
{
    (void)SendEventTelemetryUart(message);
    return *this;
}

EventLogger event{};

void SetPelengDebugSource(Peleng* peleng)
{
    (void)peleng;
}
