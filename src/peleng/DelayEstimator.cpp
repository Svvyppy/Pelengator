#include "DelayEstimator.hpp"

#include <cmath>

namespace hydrv::peleng {

using Matrix3x3 = std::array<std::array<float, 3U>, 3U>;

static constexpr float absolute(float value)
{
    return (value < 0.0f) ? -value : value;
}

static constexpr Matrix3x3 makeBaselineMatrix()
{
    Matrix3x3 matrix{};
    for (std::size_t row = 0U; row < 3U; ++row) {
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
            matrix[row][axis] = kHydrophonePositionsMeters[row + 1U][axis] - kHydrophonePositionsMeters[0][axis];
        }
    }
    return matrix;
}

static constexpr float determinant(const Matrix3x3& matrix)
{
    return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
           matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
           matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

static constexpr Matrix3x3 invert(const Matrix3x3& matrix)
{
    const float matrixDeterminant = determinant(matrix);
    return Matrix3x3{
        {
         { { (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) / matrixDeterminant,
                (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) / matrixDeterminant,
                (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) / matrixDeterminant } },
         { { (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) / matrixDeterminant,
                (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) / matrixDeterminant,
                (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) / matrixDeterminant } },
         { { (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) / matrixDeterminant,
                (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) / matrixDeterminant,
                (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) / matrixDeterminant } },
         }
    };
}

inline constexpr float kMicrosecondsPerSecond      = 1000000.0f;
inline constexpr float kDegreesPerRadian           = 57.29577951308232f;
inline constexpr float kDirectionTolerance         = 1e-6f;
inline constexpr float kMicrosecondsToSignedMeters = -kSoundSpeedMetersPerSecond / kMicrosecondsPerSecond;
inline constexpr Matrix3x3 kBaselineMatrix         = makeBaselineMatrix();
inline constexpr Matrix3x3 kInverseBaselineMatrix  = invert(kBaselineMatrix);

static_assert(absolute(determinant(kBaselineMatrix)) > 1e-9f);

static int32_t maximumDelaySamples(std::size_t firstChannel, std::size_t secondChannel)
{
    float distanceSquared = 0.0f;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
        const float delta =
            kHydrophonePositionsMeters[firstChannel][axis] - kHydrophonePositionsMeters[secondChannel][axis];
        distanceSquared += delta * delta;
    }

    const float distanceMeters = std::sqrt(distanceSquared);
    return static_cast<int32_t>(std::ceil(distanceMeters * kSampleRateHz / kSoundSpeedMetersPerSecond)) +
           kSignalDelayMarginSamples;
}

static bool arrivalTimesArePlausible(const std::array<ThresholdCrossing, kAdcChannelCount>& arrivals)
{
    for (std::size_t firstChannel = 0U; firstChannel < kAdcChannelCount; ++firstChannel) {
        for (std::size_t secondChannel = firstChannel + 1U; secondChannel < kAdcChannelCount; ++secondChannel) {
            const int32_t delay = static_cast<int32_t>(arrivals[secondChannel].sampleIndex) -
                                  static_cast<int32_t>(arrivals[firstChannel].sampleIndex);
            if (absolute(static_cast<float>(delay)) >
                static_cast<float>(maximumDelaySamples(firstChannel, secondChannel)))
            {
                return false;
            }
        }
    }
    return true;
}

ThresholdCrossing findThresholdCrossing(const SignalBlock& signal)
{
    for (std::size_t sampleIndex = 0U; sampleIndex < signal.size(); ++sampleIndex) {
        const q15_t value = signal[sampleIndex];
        if (value >= kSignalThreshold && value <= kMaximumSignalThreshold) {
            return { .sampleIndex = sampleIndex, .value = value, .found = true };
        }
    }
    return {};
}

DelayMeasurements estimateDelays(const ChannelSignalBlocks& signals)
{
    std::array<ThresholdCrossing, kAdcChannelCount> arrivals{};
    for (std::size_t channel = 0U; channel < arrivals.size(); ++channel) {
        arrivals[channel] = findThresholdCrossing(signals[channel]);
        if (!arrivals[channel].found) {
            return {};
        }
    }

    if (!arrivalTimesArePlausible(arrivals)) {
        return {};
    }

    DelayMeasurements measurements{};
    measurements.valid                = true;
    const int32_t referenceSample     = static_cast<int32_t>(arrivals[0].sampleIndex);
    measurements.channel2Samples      = static_cast<int32_t>(arrivals[1].sampleIndex) - referenceSample;
    measurements.channel3Samples      = static_cast<int32_t>(arrivals[2].sampleIndex) - referenceSample;
    measurements.channel4Samples      = static_cast<int32_t>(arrivals[3].sampleIndex) - referenceSample;
    measurements.channel2Microseconds = samplesToMicroseconds(measurements.channel2Samples);
    measurements.channel3Microseconds = samplesToMicroseconds(measurements.channel3Samples);
    measurements.channel4Microseconds = samplesToMicroseconds(measurements.channel4Samples);
    estimateDirection(measurements);
    return measurements;
}

float samplesToMicroseconds(int32_t samples)
{
    return static_cast<float>(samples) * kMicrosecondsPerSecond / kSampleRateHz;
}

void estimateDirection(DelayMeasurements& measurements)
{
    measurements.directionValid = false;
    if (!measurements.valid) {
        return;
    }

    const std::array<float, 3U> signedRanges = {
        measurements.channel2Microseconds * kMicrosecondsToSignedMeters,
        measurements.channel3Microseconds * kMicrosecondsToSignedMeters,
        measurements.channel4Microseconds * kMicrosecondsToSignedMeters,
    };

    std::array<float, 3U> direction{};
    for (std::size_t axis = 0U; axis < direction.size(); ++axis) {
        direction[axis] = kInverseBaselineMatrix[axis][0] * signedRanges[0] +
                          kInverseBaselineMatrix[axis][1] * signedRanges[1] +
                          kInverseBaselineMatrix[axis][2] * signedRanges[2];
    }

    const float norm =
        std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
    if (norm < kDirectionTolerance) {
        return;
    }

    measurements.directionX = direction[0] / norm;
    measurements.directionY = direction[1] / norm;
    measurements.directionZ = direction[2] / norm;

    const float horizontalNorm = std::sqrt(measurements.directionX * measurements.directionX +
                                           measurements.directionY * measurements.directionY);
    measurements.bearingDegrees =
        (horizontalNorm < kDirectionTolerance)
            ? 0.0f
            : std::atan2(measurements.directionY, measurements.directionX) * kDegreesPerRadian;
    measurements.elevationDegrees = std::atan2(measurements.directionZ, horizontalNorm) * kDegreesPerRadian;
    measurements.directionValid   = true;
}

} // namespace hydrv::peleng
