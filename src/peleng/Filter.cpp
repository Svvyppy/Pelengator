#include "Filter.hpp"

Filter::Filter()
{
    arm_fir_init_f32(&fir_instance_, NUM_TAPS, kEnvelopeFirCoefficients.data(), state_.data(), BLOCK_SIZE);
}

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
