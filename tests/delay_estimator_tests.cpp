#include <array>

#include <catch2/catch_test_macros.hpp>

#include "DelayEstimator.hpp"

namespace hydrv::tests {
constexpr float kMicrosecondsPerSecond = 1000000.0f;

bool NearlyEqual(float lhs, float rhs, float tolerance = 1e-6f)
{
    const float diff = lhs - rhs;
    return (diff <= tolerance) && (-diff <= tolerance);
}

hydrv::peleng::ChannelSignalBlocks MakeDetectedBuffers(
    const std::array<std::size_t, hydrv::kAdcChannelCount>& crossings)
{
    hydrv::peleng::ChannelSignalBlocks buffers{};
    for (std::size_t channel = 0U; channel < hydrv::kAdcChannelCount; ++channel) {
        buffers[channel].fill(0.0f);
        const std::size_t index = crossings[channel];
        if (index < hydrv::kSignalBlockSize) {
            buffers[channel][index] = hydrv::kSignalThreshold + 10;
        }
    }
    return buffers;
}

float ExpectedDelayMicroseconds(std::size_t hydrophone, const std::array<float, 3U>& direction)
{
    float projection_m = 0.0f;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
        projection_m +=
            (hydrv::kHydrophonePositionsMeters[hydrophone][axis] - hydrv::kHydrophonePositionsMeters[0][axis]) *
            direction[axis];
    }

    return (-projection_m / hydrv::kSoundSpeedMetersPerSecond) * kMicrosecondsPerSecond;
}
} // namespace hydrv::tests

TEST_CASE("Delay estimator computes valid inter-channel delays", "[delay]")
{
    const auto buffers                            = hydrv::tests::MakeDetectedBuffers({ 100U, 108U, 112U, 101U });
    const hydrv::peleng::DelayMeasurements delays = hydrv::peleng::estimateDelays(buffers);

    REQUIRE(delays.valid);
    REQUIRE(delays.channel2Samples == 8);
    REQUIRE(delays.channel3Samples == 12);
    REQUIRE(delays.channel4Samples == 1);
    REQUIRE(hydrv::tests::NearlyEqual(delays.channel2Microseconds, hydrv::peleng::samplesToMicroseconds(8), 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.channel3Microseconds, hydrv::peleng::samplesToMicroseconds(12), 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.channel4Microseconds, hydrv::peleng::samplesToMicroseconds(1), 1e-4f));
    REQUIRE(delays.directionValid);
    REQUIRE(hydrv::tests::NearlyEqual((delays.directionX * delays.directionX) +
                                          (delays.directionY * delays.directionY) +
                                          (delays.directionZ * delays.directionZ),
                                      1.0f,
                                      1e-4f));
}

TEST_CASE("Delay estimator marks frame invalid if any channel has no crossing", "[delay]")
{
    hydrv::peleng::ChannelSignalBlocks buffers{};
    for (auto& channel : buffers) {
        channel.fill(0.0f);
    }

    const hydrv::peleng::DelayMeasurements delays = hydrv::peleng::estimateDelays(buffers);
    REQUIRE(!delays.valid);
    REQUIRE(!delays.directionValid);
}

TEST_CASE("Threshold crossing uses only the configured envelope window", "[delay]")
{
    hydrv::peleng::SignalBlock buffer{};
    buffer.fill(0);
    buffer[10] = hydrv::kMaximumSignalThreshold + 1;
    buffer[20] = hydrv::kSignalThreshold - 1;
    buffer[30] = hydrv::kSignalThreshold;

    const hydrv::peleng::ThresholdCrossing crossing = hydrv::peleng::findThresholdCrossing(buffer);

    REQUIRE(crossing.found);
    REQUIRE(crossing.sampleIndex == 30U);
    REQUIRE(crossing.value == hydrv::kSignalThreshold);
}

TEST_CASE("Delay estimator rejects physically impossible arrival spread", "[delay]")
{
    const auto buffers                            = hydrv::tests::MakeDetectedBuffers({ 100U, 300U, 112U, 101U });
    const hydrv::peleng::DelayMeasurements delays = hydrv::peleng::estimateDelays(buffers);

    REQUIRE(!delays.valid);
    REQUIRE(!delays.directionValid);
}

TEST_CASE("Direction estimator recovers vertical source direction", "[delay]")
{
    constexpr std::array<float, 3U> kPeleng90Direction = { 0.0f, 0.0f, 1.0f };
    const float d12_us = hydrv::tests::ExpectedDelayMicroseconds(1U, kPeleng90Direction);
    const float d13_us = hydrv::tests::ExpectedDelayMicroseconds(2U, kPeleng90Direction);
    const float d14_us = hydrv::tests::ExpectedDelayMicroseconds(3U, kPeleng90Direction);

    REQUIRE(hydrv::tests::NearlyEqual(d12_us, 0.0f, 1e-4f));
    REQUIRE(d13_us < 0.0f);
    REQUIRE(hydrv::tests::NearlyEqual(d14_us, 0.0f, 1e-4f));

    hydrv::peleng::DelayMeasurements delays{};
    delays.valid                = true;
    delays.channel2Microseconds = d12_us;
    delays.channel3Microseconds = d13_us;
    delays.channel4Microseconds = d14_us;

    hydrv::peleng::estimateDirection(delays);

    REQUIRE(delays.directionValid);
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionX, 0.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionY, 0.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionZ, 1.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.bearingDegrees, 0.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.elevationDegrees, 90.0f, 1e-4f));
}

TEST_CASE("Direction estimator recovers Y positive bearing at 90 degrees", "[delay]")
{
    constexpr std::array<float, 3U> kPeleng90Direction = { 0.0f, 1.0f, 0.0f };
    const float d12_us = hydrv::tests::ExpectedDelayMicroseconds(1U, kPeleng90Direction);
    const float d13_us = hydrv::tests::ExpectedDelayMicroseconds(2U, kPeleng90Direction);
    const float d14_us = hydrv::tests::ExpectedDelayMicroseconds(3U, kPeleng90Direction);

    REQUIRE(d12_us > 0.0f);
    REQUIRE(d13_us > 0.0f);
    REQUIRE(d14_us > 0.0f);

    hydrv::peleng::DelayMeasurements delays{};
    delays.valid                = true;
    delays.channel2Microseconds = d12_us;
    delays.channel3Microseconds = d13_us;
    delays.channel4Microseconds = d14_us;

    hydrv::peleng::estimateDirection(delays);

    REQUIRE(delays.directionValid);
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionX, 0.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionY, 1.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionZ, 0.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.bearingDegrees, 90.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.elevationDegrees, 0.0f, 1e-4f));
}

TEST_CASE("Direction estimator reports elevation from XY plane", "[delay]")
{
    constexpr float kSin30                     = 0.5f;
    constexpr float kCos30                     = 0.8660254038f;
    constexpr std::array<float, 3U> kDirection = { kCos30, 0.0f, kSin30 };

    hydrv::peleng::DelayMeasurements delays{};
    delays.valid                = true;
    delays.channel2Microseconds = hydrv::tests::ExpectedDelayMicroseconds(1U, kDirection);
    delays.channel3Microseconds = hydrv::tests::ExpectedDelayMicroseconds(2U, kDirection);
    delays.channel4Microseconds = hydrv::tests::ExpectedDelayMicroseconds(3U, kDirection);

    hydrv::peleng::estimateDirection(delays);

    REQUIRE(delays.directionValid);
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionX, kCos30, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionY, 0.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.directionZ, kSin30, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.bearingDegrees, 0.0f, 1e-4f));
    REQUIRE(hydrv::tests::NearlyEqual(delays.elevationDegrees, 30.0f, 1e-4f));
}
