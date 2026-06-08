#pragma once

#include <array>
#include <cstddef>

extern "C"
{
#include <arm_math.h>
}

#include "CommonSettings.h"

namespace hydrv::peleng {

class Filter
{
public:
    Filter();
    void apply(const q15_t* input, q15_t* output, std::size_t sampleCount);

private:
    arm_fir_instance_q15 _filter{};
    std::array<q15_t, kFilterBlockSize + kEnvelopeFilterTapCount> _state{};
};

} // namespace hydrv::peleng
