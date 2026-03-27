#include "Filter.hpp"

namespace
{
constexpr std::array<float32_t, NUM_TAPS> kFirCoeffs = {
    -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f,
    -0.0173976984f, -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f,
    +0.2504960933f, +0.2229246956f, +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f,
    -0.0173976984f, -0.0000000000f, +0.0085302217f, +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f,
    -0.0018225230f, +0.0000000000f, +0.0000000000f, +0.0000000000f};
}

Filter::Filter() { arm_fir_init_f32(&fir_instance_, NUM_TAPS, kFirCoeffs.data(), state_.data(), BLOCK_SIZE); }

void Filter::ApplyEnvelope(const float *input, float *output, std::size_t sample_count)
{
    arm_mult_f32(input, input, square_buffer_.data(), sample_count);

    const std::size_t block_count = sample_count / BLOCK_SIZE;
    for (std::size_t block = 0; block < block_count; ++block)
    {
        const std::size_t offset = block * BLOCK_SIZE;
        arm_fir_f32(&fir_instance_, square_buffer_.data() + offset, output + offset, BLOCK_SIZE);
    }
}
