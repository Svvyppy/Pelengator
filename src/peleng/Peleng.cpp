#include "Peleng.hpp"

Peleng::Peleng() = default;

void Peleng::Init()
{
    for (auto& buffer : sample_buffers_) {
        buffer.fill(0);
    }
    platform::StartSignalAcquisition(&sample_buffers_);
}

bool Peleng::Process()
{
    platform::SignalBlock block = platform::SignalBlock::FirstHalf;
    if (platform::ConsumeReadySignalBlock(&block)) {
        const std::size_t start_index = (block == platform::SignalBlock::FirstHalf) ? 0U : SIGNAL_BLOCK_SIZE;
        ProcessHalfTransfer(start_index);
        return true;
    }

    return false;
}

bool Peleng::TryGetLatestDelays(DelayMeasurements* out)
{
    if (!has_new_delays_ || out == nullptr) {
        return false;
    }

    *out            = latest_delays_;
    has_new_delays_ = false;
    return true;
}

void Peleng::FillDebugSnapshot(platform::SignalAcquisitionDebug* out) const
{
    platform::FillSignalAcquisitionDebug(out, sample_buffers_);
}

void Peleng::ProcessHalfTransfer(std::size_t start_index)
{
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel) {
        ConvertAdcToSquaredQ15(
            sample_buffers_[channel].data() + start_index, work_buffers_[channel].data(), SIGNAL_BLOCK_SIZE);
        envelope_filters_[channel].ApplyFir(
            work_buffers_[channel].data(), envelope_buffers_[channel].data(), SIGNAL_BLOCK_SIZE);
    }

    latest_delays_  = peleng::EstimateDelayMeasurements(envelope_buffers_);
    has_new_delays_ = true;
}

void Peleng::ConvertAdcToSquaredQ15(const uint16_t* source, q15_t* destination, std::size_t length)
{
    constexpr uint32_t kAdcMask12Bit = 0x0FFFU;
    constexpr int32_t kAdcMidpoint   = 2048;
    constexpr int32_t kQ15Shift      = 4;
    constexpr int32_t kQ15Max        = 32767;

    for (std::size_t index = 0U; index < length; ++index) {
        const int32_t raw        = static_cast<int32_t>(source[index] & kAdcMask12Bit);
        const int32_t sample_q15 = (raw - kAdcMidpoint) << kQ15Shift;
        const int32_t squared    = (sample_q15 * sample_q15) >> 15;
        destination[index]       = static_cast<q15_t>((squared > kQ15Max) ? kQ15Max : squared);
    }
}
