#include "DelayEstimator.hpp"

#include <cmath>

namespace {
constexpr float kMicrosecondsPerSecond = 1000000.0f;
constexpr float kDegreesPerRadian = 57.29577951308232f;
constexpr float kSingularTolerance = 1e-9f;
constexpr float kNormTolerance = 1e-6f;

float Determinant3x3(const float matrix[3][3])
{
    return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
           matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
           matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

bool Solve3x3(const float matrix[3][3], const float rhs[3], float out[3])
{
    const float determinant = Determinant3x3(matrix);
    if (std::fabs(determinant) < kSingularTolerance)
    {
        return false;
    }

    float replaced[3][3] = {};
    for (std::size_t column = 0U; column < 3U; ++column)
    {
        for (std::size_t row = 0U; row < 3U; ++row)
        {
            for (std::size_t copy_column = 0U; copy_column < 3U; ++copy_column)
            {
                replaced[row][copy_column] = (copy_column == column) ? rhs[row] : matrix[row][copy_column];
            }
        }
        out[column] = Determinant3x3(replaced) / determinant;
    }

    return true;
}
}

namespace peleng {
ThresholdCrossing FindThresholdCrossing(const HalfBuffer& buffer)
{
    for (std::size_t index = 0U; index < SIGNAL_BLOCK_SIZE; ++index) {
        const q15_t value = buffer[index];
        if (value > SIGNAL_THRESHOLD_Q15) {
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

    const float delays_s[ADC_CHANNELS - 1U] = {
        measurements.d12_us / kMicrosecondsPerSecond,
        measurements.d13_us / kMicrosecondsPerSecond,
        measurements.d14_us / kMicrosecondsPerSecond,
    };

    float normal_matrix[3][3] = {};
    float normal_rhs[3] = {};

    for (std::size_t row = 0U; row < ADC_CHANNELS - 1U; ++row)
    {
        const std::size_t hydrophone = row + 1U;
        const float rhs = -SOUND_SPEED_MPS * delays_s[row];
        float geometry_row[3] = {};
        for (std::size_t axis = 0U; axis < 3U; ++axis)
        {
            geometry_row[axis] = HYDROPHONE_POSITIONS_M[hydrophone][axis] - HYDROPHONE_POSITIONS_M[0][axis];
            normal_rhs[axis] += geometry_row[axis] * rhs;
        }

        for (std::size_t lhs_axis = 0U; lhs_axis < 3U; ++lhs_axis)
        {
            for (std::size_t rhs_axis = 0U; rhs_axis < 3U; ++rhs_axis)
            {
                normal_matrix[lhs_axis][rhs_axis] += geometry_row[lhs_axis] * geometry_row[rhs_axis];
            }
        }
    }

    float direction[3] = {};
    if (!Solve3x3(normal_matrix, normal_rhs, direction))
    {
        return;
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
    measurements.peleng_deg = std::atan2(measurements.direction_y, measurements.direction_x) * kDegreesPerRadian;
    measurements.azimuth_deg = measurements.peleng_deg;
    measurements.elevation_deg =
        std::atan2(measurements.direction_z,
                   std::sqrt(measurements.direction_x * measurements.direction_x +
                             measurements.direction_y * measurements.direction_y)) *
        kDegreesPerRadian;
    measurements.angles_valid = true;
}
} // namespace peleng
