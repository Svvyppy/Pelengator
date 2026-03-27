#ifdef __cplusplus
extern "C"
{
#endif
#include <arm_math.h>
#ifdef __cplusplus
}
#endif
#include <array>

#include "CommonSettings.h"

static const float32_t firCoeffs32[NUM_TAPS] = {
    -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f,
    -0.0000000000f, -0.0173976984f, -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f,
    +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f, +0.1522061835f, +0.0676308395f,
    +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
    +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f};

class Filter
{
public:
    Filter() { arm_fir_init_f32(_firInstance, NUM_TAPS, firCoeffs32, _state.data(), BLOCK_SIZE); }

    std::array<float32_t, BUFFER_SIZE> applyFilter(const std::array<float32_t, BUFFER_SIZE> &input)
    {
        std::array<float32_t, BUFFER_SIZE> output;
        for (int i = 0; i < NUM_BLOCKS; i++)
        {
            arm_fir_f32(_firInstance, input.data() + (i * BLOCK_SIZE), output.data() + (i * BLOCK_SIZE), BLOCK_SIZE);
        }
        return output;
    }

private:
    arm_fir_instance_f32 *_firInstance;
    std::array<float32_t, BLOCK_SIZE + NUM_TAPS - 1> _state;
};