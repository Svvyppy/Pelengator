#include "Filter.hpp"

Filter::Filter()
{
    static_assert((Q15_NUM_TAPS % 2U) == 0U);
    static_assert(Q15_NUM_TAPS >= 4U);
    static_assert((SIGNAL_BLOCK_SIZE % BLOCK_SIZE) == 0U);
    (void)arm_fir_init_q15(&fir_instance_, Q15_NUM_TAPS, kEnvelopeFirCoefficientsQ15.data(), state_.data(),
                           BLOCK_SIZE);
}

void Filter::ApplyEnvelope(const q15_t *input, q15_t *output, std::size_t sample_count)
{
    if ((input == nullptr) || (output == nullptr) || (sample_count > SIGNAL_BLOCK_SIZE) ||
        ((sample_count % BLOCK_SIZE) != 0U)) {
        return;
    }

    arm_mult_q15(input, input, square_buffer_.data(), sample_count);
    ApplyFir(square_buffer_.data(), output, sample_count);
}

void Filter::ApplyFir(const q15_t *input, q15_t *output, std::size_t sample_count)
{
    if ((input == nullptr) || (output == nullptr) || (sample_count > SIGNAL_BLOCK_SIZE) ||
        ((sample_count % BLOCK_SIZE) != 0U)) {
        return;
    }

    const std::size_t block_count = sample_count / BLOCK_SIZE;
    for (std::size_t block = 0; block < block_count; ++block) {
        const std::size_t offset = block * BLOCK_SIZE;
        arm_fir_q15(&fir_instance_, input + offset, output + offset, BLOCK_SIZE);
    }
}
