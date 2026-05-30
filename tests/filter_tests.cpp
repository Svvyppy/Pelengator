#include <array>

#include <catch2/catch_test_macros.hpp>

#include "Filter.hpp"
#include "FilterCoefficients.hpp"

namespace
{
bool NearlyEqual(q15_t lhs, q15_t rhs, q15_t tolerance = 1)
{
    const int diff = static_cast<int>(lhs) - static_cast<int>(rhs);
    return (diff <= tolerance) && (-diff <= tolerance);
}
} // namespace

TEST_CASE("Envelope filter reaches expected steady-state for constant input", "[filter]")
{
    Filter filter;

    std::array<q15_t, DMA_HALF_BUFFER_SIZE> input{};
    std::array<q15_t, DMA_HALF_BUFFER_SIZE> output{};
    input.fill(16384);
    output.fill(0);

    filter.ApplyEnvelope(input.data(), output.data(), input.size());

    constexpr q15_t expected = 8192;
    REQUIRE(NearlyEqual(output[Q15_NUM_TAPS], expected));
    REQUIRE(NearlyEqual(output.back(), expected));
}

TEST_CASE("Envelope filter returns zero for zero input", "[filter]")
{
    Filter filter;

    std::array<q15_t, DMA_HALF_BUFFER_SIZE> input{};
    std::array<q15_t, DMA_HALF_BUFFER_SIZE> output{};
    input.fill(0);
    output.fill(123);

    filter.ApplyEnvelope(input.data(), output.data(), input.size());

    for (const q15_t value : output)
    {
        REQUIRE(value == 0);
    }
}
