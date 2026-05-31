#include "DelayEstimator.hpp"

#include <cmath>

namespace {
constexpr float kMicrosecondsPerSecond = 1000000.0f;
constexpr float kDegreesPerRadian = 57.29577951308232f;
constexpr float kNormTolerance = 1e-6f;
constexpr float kDelayMicrosecondsToSignedMeters = -SOUND_SPEED_MPS / kMicrosecondsPerSecond;

using Matrix3x3 = std::array<std::array<float, 3U>, 3U>;

constexpr float Abs(float value)
{
    return (value < 0.0f) ? -value : value;
}

constexpr Matrix3x3 MakeBaselineMatrix()
{
    Matrix3x3 matrix{};
    for (std::size_t row = 0U; row < 3U; ++row)
    {
        for (std::size_t axis = 0U; axis < 3U; ++axis)
        {
            matrix[row][axis] = HYDROPHONE_POSITIONS_M[row + 1U][axis] - HYDROPHONE_POSITIONS_M[0][axis];
        }
    }

    return matrix;
}

constexpr float Determinant3x3(const Matrix3x3& matrix)
{
    return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
           matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
           matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

constexpr Matrix3x3 Inverse3x3(const Matrix3x3& matrix)
{
    const float determinant = Determinant3x3(matrix);
    return Matrix3x3{{
        {{(matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) / determinant,
          (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) / determinant,
          (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) / determinant}},
        {{(matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) / determinant,
          (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) / determinant,
          (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) / determinant}},
        {{(matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) / determinant,
          (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) / determinant,
          (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) / determinant}},
    }};
}

constexpr Matrix3x3 kBaselineMatrix = MakeBaselineMatrix();
static_assert(Abs(Determinant3x3(kBaselineMatrix)) > 1e-9f);
constexpr Matrix3x3 kInverseBaselineMatrix = Inverse3x3(kBaselineMatrix);

int32_t MaxDelaySamplesBetween(std::size_t first_channel, std::size_t second_channel)
{
    float distance_squared = 0.0f;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
        const float delta = HYDROPHONE_POSITIONS_M[first_channel][axis] -
                            HYDROPHONE_POSITIONS_M[second_channel][axis];
        distance_squared += delta * delta;
    }

    const float distance_m = std::sqrt(distance_squared);
    const float samples = (distance_m * SAMPLE_RATE_HZ) / SOUND_SPEED_MPS;
    return static_cast<int32_t>(std::ceil(samples)) + SIGNAL_DELAY_MARGIN_SAMPLES;
}

bool AreArrivalTimesPhysicallyPlausible(const std::array<ThresholdCrossing, ADC_CHANNELS>& arrivals)
{
    for (std::size_t first_channel = 0U; first_channel < ADC_CHANNELS; ++first_channel) {
        for (std::size_t second_channel = first_channel + 1U; second_channel < ADC_CHANNELS; ++second_channel) {
            const auto first_index = static_cast<int32_t>(arrivals[first_channel].index);
            const auto second_index = static_cast<int32_t>(arrivals[second_channel].index);
            const int32_t delay_samples = second_index - first_index;
            if (Abs(static_cast<float>(delay_samples)) >
                static_cast<float>(MaxDelaySamplesBetween(first_channel, second_channel))) {
                return false;
            }
        }
    }

    return true;
}
}

namespace peleng {
ThresholdCrossing FindThresholdCrossing(const HalfBuffer& buffer)
{
    for (std::size_t index = 0U; index < SIGNAL_BLOCK_SIZE; ++index) {
        const q15_t value = buffer[index];
        if ((value >= SIGNAL_THRESHOLD_Q15) && (value <= SIGNAL_THRESHOLD_MAX_Q15)) {
            return ThresholdCrossing{ .index = index, .value = value, .found = true };
        }
    }

    return ThresholdCrossing{};
}

DelayMeasurements EstimateDelayMeasurements(const EnvelopeBuffers& buffers)
{
    std::array<ThresholdCrossing, ADC_CHANNELS> arrivals{};
    for (std::size_t channel = 0U; channel < ADC_CHANNELS; ++channel) {
        arrivals[channel] = FindThresholdCrossing(buffers[channel]);
    }

    DelayMeasurements current{};
    current.valid = true;
    for (const auto& arrival : arrivals) {
        if (!arrival.found) {
            current.valid = false;
            break;
        }
    }

    if (!current.valid) {
        return current;
    }

    if (!AreArrivalTimesPhysicallyPlausible(arrivals)) {
        current.valid = false;
        return current;
    }

    const auto ref_index = static_cast<int32_t>(arrivals[0].index);
    current.d12_samples  = static_cast<int32_t>(arrivals[1].index) - ref_index;
    current.d13_samples  = static_cast<int32_t>(arrivals[2].index) - ref_index;
    current.d14_samples  = static_cast<int32_t>(arrivals[3].index) - ref_index;
    current.d12_us       = SamplesToMicroseconds(current.d12_samples);
    current.d13_us       = SamplesToMicroseconds(current.d13_samples);
    current.d14_us       = SamplesToMicroseconds(current.d14_samples);
    EstimateDirectionLeastSquares(current);
    return current;
}

float SamplesToMicroseconds(int32_t samples)
{
    const auto samples_f = static_cast<float>(samples);
    return (samples_f * kMicrosecondsPerSecond) / SAMPLE_RATE_HZ;
}

void EstimateDirectionLeastSquares(DelayMeasurements& measurements)
{
    measurements.angles_valid = false;
    if (!measurements.valid)
    {
        return;
    }

    const float signed_ranges_m[ADC_CHANNELS - 1U] = {
        measurements.d12_us * kDelayMicrosecondsToSignedMeters,
        measurements.d13_us * kDelayMicrosecondsToSignedMeters,
        measurements.d14_us * kDelayMicrosecondsToSignedMeters,
    };

    float direction[3] = {};
    for (std::size_t axis = 0U; axis < 3U; ++axis)
    {
        direction[axis] = (kInverseBaselineMatrix[axis][0] * signed_ranges_m[0]) +
                          (kInverseBaselineMatrix[axis][1] * signed_ranges_m[1]) +
                          (kInverseBaselineMatrix[axis][2] * signed_ranges_m[2]);
    }

    const float norm =
        std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
    if (norm < kNormTolerance)
    {
        return;
    }

    measurements.direction_x = direction[0] / norm;
    measurements.direction_y = direction[1] / norm;
    measurements.direction_z = direction[2] / norm;

    const float horizontal_norm =
        std::sqrt(measurements.direction_x * measurements.direction_x +
                  measurements.direction_y * measurements.direction_y);
    measurements.peleng_deg = (horizontal_norm < kNormTolerance)
                                  ? 0.0f
                                  : std::atan2(measurements.direction_y, measurements.direction_x) *
                                        kDegreesPerRadian;
    measurements.elevation_deg = std::atan2(measurements.direction_z, horizontal_norm) * kDegreesPerRadian;
    measurements.angles_valid = true;
}
} // namespace peleng
