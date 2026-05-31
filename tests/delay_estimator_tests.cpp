#include <array>

#include <catch2/catch_test_macros.hpp>

#include "DelayEstimator.hpp"

namespace
{
constexpr float kMicrosecondsPerSecond = 1000000.0f;

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
        if (index < SIGNAL_BLOCK_SIZE)
        {
            buffers[channel][index] = SIGNAL_THRESHOLD_Q15 + 10;
        }
    }
    return buffers;
}

float ExpectedDelayMicroseconds(std::size_t hydrophone, const std::array<float, 3U> &direction)
{
    float projection_m = 0.0f;
    for (std::size_t axis = 0U; axis < 3U; ++axis)
    {
        projection_m += (HYDROPHONE_POSITIONS_M[hydrophone][axis] - HYDROPHONE_POSITIONS_M[0][axis]) *
                        direction[axis];
    }

    return (-projection_m / SOUND_SPEED_MPS) * kMicrosecondsPerSecond;
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
    REQUIRE(delays.angles_valid);
    REQUIRE(NearlyEqual((delays.direction_x * delays.direction_x) + (delays.direction_y * delays.direction_y) +
                            (delays.direction_z * delays.direction_z),
                        1.0f, 1e-4f));
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
    REQUIRE(!delays.angles_valid);
}

TEST_CASE("Direction estimator recovers vertical source direction", "[delay]")
{
    constexpr std::array<float, 3U> kPeleng90Direction = {0.0f, 0.0f, 1.0f};
    const float d12_us = ExpectedDelayMicroseconds(1U, kPeleng90Direction);
    const float d13_us = ExpectedDelayMicroseconds(2U, kPeleng90Direction);
    const float d14_us = ExpectedDelayMicroseconds(3U, kPeleng90Direction);

    REQUIRE(NearlyEqual(d12_us, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(d13_us, -146.66667f, 1e-4f));
    REQUIRE(NearlyEqual(d14_us, 0.0f, 1e-4f));

    DelayMeasurements delays{};
    delays.valid = true;
    delays.d12_us = d12_us;
    delays.d13_us = d13_us;
    delays.d14_us = d14_us;

    peleng::EstimateDirectionLeastSquares(delays);

    REQUIRE(delays.angles_valid);
    REQUIRE(NearlyEqual(delays.direction_x, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.direction_y, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.direction_z, 1.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.peleng_deg, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.elevation_deg, 90.0f, 1e-4f));
}

TEST_CASE("Direction estimator recovers Y positive bearing at 90 degrees", "[delay]")
{
    constexpr std::array<float, 3U> kPeleng90Direction = {0.0f, 1.0f, 0.0f};
    const float d12_us = ExpectedDelayMicroseconds(1U, kPeleng90Direction);
    const float d13_us = ExpectedDelayMicroseconds(2U, kPeleng90Direction);
    const float d14_us = ExpectedDelayMicroseconds(3U, kPeleng90Direction);

    REQUIRE(NearlyEqual(d12_us, 156.66667f, 1e-4f));
    REQUIRE(NearlyEqual(d13_us, 103.0f, 1e-4f));
    REQUIRE(NearlyEqual(d14_us, 78.33334f, 1e-4f));

    DelayMeasurements delays{};
    delays.valid = true;
    delays.d12_us = d12_us;
    delays.d13_us = d13_us;
    delays.d14_us = d14_us;

    peleng::EstimateDirectionLeastSquares(delays);

    REQUIRE(delays.angles_valid);
    REQUIRE(NearlyEqual(delays.direction_x, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.direction_y, 1.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.direction_z, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.peleng_deg, 90.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.elevation_deg, 0.0f, 1e-4f));
}

TEST_CASE("Direction estimator reports elevation from XY plane", "[delay]")
{
    constexpr float kSin30 = 0.5f;
    constexpr float kCos30 = 0.8660254038f;
    constexpr std::array<float, 3U> kDirection = {kCos30, 0.0f, kSin30};

    DelayMeasurements delays{};
    delays.valid = true;
    delays.d12_us = ExpectedDelayMicroseconds(1U, kDirection);
    delays.d13_us = ExpectedDelayMicroseconds(2U, kDirection);
    delays.d14_us = ExpectedDelayMicroseconds(3U, kDirection);

    peleng::EstimateDirectionLeastSquares(delays);

    REQUIRE(delays.angles_valid);
    REQUIRE(NearlyEqual(delays.direction_x, kCos30, 1e-4f));
    REQUIRE(NearlyEqual(delays.direction_y, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.direction_z, kSin30, 1e-4f));
    REQUIRE(NearlyEqual(delays.peleng_deg, 0.0f, 1e-4f));
    REQUIRE(NearlyEqual(delays.elevation_deg, 30.0f, 1e-4f));
}
