#include <array>

#include <catch2/catch_test_macros.hpp>

#include "Filter.hpp"
#include "FilterCoefficients.hpp"

namespace hydrv::tests {
bool NearlyEqual(q15_t lhs, q15_t rhs, q15_t tolerance = 1)
{
    const int diff = static_cast<int>(lhs) - static_cast<int>(rhs);
    return (diff <= tolerance) && (-diff <= tolerance);
}
} // namespace hydrv::tests

TEST_CASE("Envelope filter reaches expected steady-state for constant input", "[filter]")
{
    hydrv::peleng::Filter filter;

    std::array<q15_t, hydrv::kSignalBlockSize> input{};
    std::array<q15_t, hydrv::kSignalBlockSize> output{};
    input.fill(8192);
    output.fill(0);

    filter.apply(input.data(), output.data(), input.size());

    constexpr q15_t expected = 8192;
    REQUIRE(hydrv::tests::NearlyEqual(output[hydrv::kEnvelopeFilterTapCount], expected));
    REQUIRE(hydrv::tests::NearlyEqual(output.back(), expected));
}

TEST_CASE("Envelope filter returns zero for zero input", "[filter]")
{
    hydrv::peleng::Filter filter;

    std::array<q15_t, hydrv::kSignalBlockSize> input{};
    std::array<q15_t, hydrv::kSignalBlockSize> output{};
    input.fill(0);
    output.fill(123);

    filter.apply(input.data(), output.data(), input.size());

    for (const q15_t value : output) {
        REQUIRE(value == 0);
    }
}
