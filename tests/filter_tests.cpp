#include <array>
#include <numeric>

#include <catch2/catch_test_macros.hpp>

#include "Filter.hpp"
#include "FilterCoefficients.hpp"

namespace
{
bool NearlyEqual(float lhs, float rhs, float tolerance = 1e-6f)
{
    const float diff = lhs - rhs;
    return (diff <= tolerance) && (-diff <= tolerance);
}
} // namespace

TEST_CASE("Envelope filter reaches expected steady-state for constant input", "[filter]")
{
    Filter filter;

    std::array<float, DMA_HALF_BUFFER_SIZE> input{};
    std::array<float, DMA_HALF_BUFFER_SIZE> output{};
    input.fill(1.5f);
    output.fill(0.0f);

    filter.ApplyEnvelope(input.data(), output.data(), input.size());

    const float coeff_sum =
        std::accumulate(kEnvelopeFirCoefficients.begin(), kEnvelopeFirCoefficients.end(), 0.0f);
    const float expected = 1.5f * 1.5f * coeff_sum;

    REQUIRE(NearlyEqual(expected, 2.25f, 1e-3f));
    REQUIRE(NearlyEqual(output[NUM_TAPS], expected, 1e-3f));
    REQUIRE(NearlyEqual(output.back(), expected, 1e-3f));
}

TEST_CASE("Envelope filter returns zero for zero input", "[filter]")
{
    Filter filter;

    std::array<float, DMA_HALF_BUFFER_SIZE> input{};
    std::array<float, DMA_HALF_BUFFER_SIZE> output{};
    input.fill(0.0f);
    output.fill(123.0f);

    filter.ApplyEnvelope(input.data(), output.data(), input.size());

    for (const float value : output)
    {
        REQUIRE(NearlyEqual(value, 0.0f));
    }
}
