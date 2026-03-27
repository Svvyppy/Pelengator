#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <iostream>
#include <string_view>

#include "DelayEstimator.hpp"
#include "Filter.hpp"
#include "FilterCoefficients.hpp"

namespace
{
int g_failures = 0;

bool NearlyEqual(float lhs, float rhs, float tolerance = 1e-6f)
{
    return std::fabs(lhs - rhs) <= tolerance;
}

void Check(bool condition, std::string_view message)
{
    if (!condition)
    {
        ++g_failures;
        std::cerr << "[FAIL] " << message << '\n';
    }
}

void TestFilterConstantInput()
{
    Filter filter;

    std::array<float, DMA_HALF_BUFFER_SIZE> input{};
    std::array<float, DMA_HALF_BUFFER_SIZE> output{};
    input.fill(0.0f);
    output.fill(0.0f);
    input.fill(1.5f);

    filter.ApplyEnvelope(input.data(), output.data(), input.size());

    const float expected = 1.5f * 1.5f * std::accumulate(kEnvelopeFirCoefficients.begin(),
                                                         kEnvelopeFirCoefficients.end(), 0.0f);
    Check(NearlyEqual(expected, 2.25f, 1e-3f), "Envelope FIR coefficients should be normalized");
    Check(NearlyEqual(output[NUM_TAPS], expected, 1e-3f), "Filter steady-state output mismatch");
    Check(NearlyEqual(output.back(), expected, 1e-3f), "Filter end-of-block output mismatch");
}

void TestFilterZeroInput()
{
    Filter filter;

    std::array<float, DMA_HALF_BUFFER_SIZE> input{};
    std::array<float, DMA_HALF_BUFFER_SIZE> output{};
    input.fill(0.0f);
    output.fill(123.0f);

    filter.ApplyEnvelope(input.data(), output.data(), input.size());

    for (float value : output)
    {
        Check(NearlyEqual(value, 0.0f), "Zero input must produce zero output");
    }
}

peleng::EnvelopeBuffers MakeDetectedBuffers(std::array<std::size_t, ADC_CHANNELS> crossings)
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

void TestDelayEstimatorValid()
{
    const auto buffers = MakeDetectedBuffers({100U, 108U, 112U, 101U});
    const DelayMeasurements delays = peleng::EstimateDelayMeasurements(buffers);

    Check(delays.valid, "Delay estimator should report valid frame");
    Check(delays.d12_samples == 8, "d12 sample delay mismatch");
    Check(delays.d13_samples == 12, "d13 sample delay mismatch");
    Check(delays.d14_samples == 1, "d14 sample delay mismatch");
    Check(NearlyEqual(delays.d12_us, peleng::SamplesToMicroseconds(8), 1e-4f), "d12 microseconds mismatch");
    Check(NearlyEqual(delays.d13_us, peleng::SamplesToMicroseconds(12), 1e-4f), "d13 microseconds mismatch");
    Check(NearlyEqual(delays.d14_us, peleng::SamplesToMicroseconds(1), 1e-4f), "d14 microseconds mismatch");
}

void TestDelayEstimatorInvalid()
{
    peleng::EnvelopeBuffers buffers{};
    for (auto &channel : buffers)
    {
        channel.fill(0.0f);
    }

    const DelayMeasurements delays = peleng::EstimateDelayMeasurements(buffers);
    Check(!delays.valid, "Delay estimator should mark empty frame as invalid");
}

} // namespace

int main()
{
    TestFilterConstantInput();
    TestFilterZeroInput();
    TestDelayEstimatorValid();
    TestDelayEstimatorInvalid();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All host DSP tests passed\n";
    return 0;
}
