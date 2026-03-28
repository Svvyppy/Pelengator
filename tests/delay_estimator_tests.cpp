#include <array>

#include <catch2/catch_test_macros.hpp>

#include "DelayEstimator.hpp"

namespace
{
bool NearlyEqual(float lhs, float rhs, float tolerance = 1e-6f)
{
    const float diff = lhs - rhs;
    return (diff <= tolerance) && (-diff <= tolerance);
}

peleng::EnvelopeBuffers MakeDetectedBuffers(const std::array<std::size_t, ADC_CHANNELS> &crossings)
{
    peleng::EnvelopeBuffers buffers{};
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel)
    {
        buffers[channel].fill(0.0f);
        const std::size_t index = crossings[channel];
        if (index < DMA_HALF_BUFFER_SIZE)
        {
            buffers[channel][index] = SIGNAL_THRESHOLD + 10.0f;
        }
    }
    return buffers;
}
} // namespace

TEST_CASE("Delay estimator computes valid inter-channel delays", "[delay]")
{
    const auto buffers = MakeDetectedBuffers({100U, 108U, 112U, 101U});
    const DelayMeasurements delays = peleng::EstimateDelayMeasurements(buffers);

    REQUIRE(delays.valid);
    REQUIRE(delays.d12_samples == 8);
    REQUIRE(delays.d13_samples == 12);
    REQUIRE(delays.d14_samples == 1);
    REQUIRE(NearlyEqual(delays.d12_us, peleng::SamplesToMicroseconds(8), 1e-4f));
    REQUIRE(NearlyEqual(delays.d13_us, peleng::SamplesToMicroseconds(12), 1e-4f));
    REQUIRE(NearlyEqual(delays.d14_us, peleng::SamplesToMicroseconds(1), 1e-4f));
}

TEST_CASE("Delay estimator marks frame invalid if any channel has no crossing", "[delay]")
{
    peleng::EnvelopeBuffers buffers{};
    for (auto &channel : buffers)
    {
        channel.fill(0.0f);
    }

    const DelayMeasurements delays = peleng::EstimateDelayMeasurements(buffers);
    REQUIRE(!delays.valid);
}
