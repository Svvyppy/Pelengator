#include "Filter.hpp"

#include "FilterCoefficients.hpp"

namespace hydrv::peleng {

Filter::Filter()
{
    static_assert((kEnvelopeFilterTapCount % 2U) == 0U);
    static_assert((kSignalBlockSize % kFilterBlockSize) == 0U);
    (void)arm_fir_init_q15(
        &_filter, kEnvelopeFilterTapCount, kEnvelopeFilterCoefficients.data(), _state.data(), kFilterBlockSize);
}

void Filter::apply(const q15_t* input, q15_t* output, std::size_t sampleCount)
{
    if (input == nullptr || output == nullptr || sampleCount > kSignalBlockSize ||
        (sampleCount % kFilterBlockSize) != 0U)
    {
        return;
    }

    for (std::size_t offset = 0U; offset < sampleCount; offset += kFilterBlockSize) {
        arm_fir_q15(&_filter, input + offset, output + offset, kFilterBlockSize);
    }
}

} // namespace hydrv::peleng
