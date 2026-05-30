#pragma once

#include <array>

extern "C"
{
#include <arm_math.h>
}

#include "CommonSettings.h"

inline constexpr std::array<q15_t, Q15_NUM_TAPS> kEnvelopeFirCoefficientsQ15 = {

    240, 348, 652, 1128, 1722, 2355, 2936, 3381, 3622, 3622, 3381, 2936, 2355, 1722, 1128, 652, 348, 240
};
