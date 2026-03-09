#include "NonUniformConvolver.h"

#include <cstring>
#include <cmath>
#include <algorithm>

#if defined(__AVX__) || defined(__AVX2__)
  #include <immintrin.h>
  #define NUC_HAS_AVX 1
#endif

// ---------------------------------------------------------------------------
// JUCE FFT buffer contract — verified against juce_FFT.cpp source:
//
//   Buffer size: 2 * fftSize_ floats always required.
//
//   performRealOnlyForwardTransform(buf, false):
//     Input:  real samples in buf[0..fftSize_-1], zeros in upper half.
//     Output: fftSize_ interleaved complex pairs across buf[0..2*fftSize_-1]:
//               buf[0],buf[1]                 = DC       (re, im)
//               buf[2k],buf[2k+1]             = bin k    (re, im)  k=1..fftSize_/2-1
//               buf[fftSize_],buf[fftSize_+1] = Nyquist  (re, im)
//               buf[fftSize_+2..]             = conjugate mirrors (not needed for IFFT)
//     No normalisation applied.
//
//   performRealOnlyInverseTransform(buf):
//     Reads bins 0..fftSize_/2 (DC through Nyquist) from buf[0..fftSize_+1].
//     Mirrors negative frequencies internally.
//     Writes real output to buf[0..fftSize_-1]  (FIRST HALF).
//     Writes imag residuals to buf[fftSize_..2*fftSize_-1] (ignore these).
//     Normalises by 1/fftSize_ automatically — do NOT divide again.
//
// Overlap-add (OLA) per block:
//   1. Zero-pad input[0..B-1] to fftSize_ → FFT → store in FDL.
//   2. complexMultiplyAdd over all partitions → accumBuf_.
//   3. IFFT → fftSize_ real samples in accumBuf_[0..fftSize_-1].
//   4. output[n] = accumBuf_[n] + overlapBuf_[n]   n=0..B-1
//   5. overlapBuf_ = accumBuf_[B..2B-1]
// ---------------------------------------------------------------------------

void NonUniformConvolver::prepare(double /*sampleRate*/, int blockSize)
{
    jassert(blockSize > 0);
    jassert((blockSize & (blockSize - 1)) == 0);

    blockSize_ = blockSize;
    fftSize_   = blockSize * 2;

    fftOrder_ = 0;
    for (int v = fftSize_; v > 1; v >>= 1)
        ++fftOrder_;

    fft_ = std::make_unique<juce::dsp::FFT>(fftOrder_);

    accumBuf_  .assign(2 * fftSize_, 0.0f);
    tempBuf_   .assign(2 * fftSize_, 0.0f);
    overlapBuf_.assign(blockSize_,   0.0f);

    numPartitions_ = 0;
    fdlDepth_      = 0;
    fdlHead_       = 0;
    tailCycleIdx_  = 0;
}

void NonUniformConvolver::loadImpulseResponse(const std::vector<float>& ir)
{
    if (blockSize_ == 0 || ir.empty())
        return;

    numPartitions_ = (int)((ir.size() + (size_t)blockSize_ - 1) / (size_t)blockSize_);
    H_.resize(numPartitions_);

    for (int p = 0; p < numPartitions_; ++p)
    {
        H_[p].assign(2 * fftSize_, 0.0f);

        const int start = p * blockSize_;
        const int count = std::min(blockSize_, (int)ir.size() - start);
        for (int n = 0; n < count; ++n)
            H_[p][n] = ir[(size_t)(start + n)];

        fft_->performRealOnlyForwardTransform(H_[p].data());
        // H_[p] now contains the full 2*fftSize_ interleaved complex buffer.
        // Only bins 0..fftSize_/2 (indices 0..fftSize_+1) are needed for multiply.
    }

    fdlDepth_ = std::max(numPartitions_, 1);
    fdl_.assign(fdlDepth_, std::vector<float>(2 * fftSize_, 0.0f));

    reset();
}

void NonUniformConvolver::reset()
{
    std::fill(accumBuf_  .begin(), accumBuf_  .end(), 0.0f);
    std::fill(tempBuf_   .begin(), tempBuf_   .end(), 0.0f);
    std::fill(overlapBuf_.begin(), overlapBuf_.end(), 0.0f);

    for (auto& slot : fdl_)
        std::fill(slot.begin(), slot.end(), 0.0f);

    fdlHead_      = 0;
    tailCycleIdx_ = 0;
}

void NonUniformConvolver::processBlock(const float* input, float* output, int numSamples)
{
    jassert(numSamples > 0 && numSamples <= blockSize_);
    const int validIn = std::min(numSamples, blockSize_);

    if (numPartitions_ == 0 || !fft_)
    {
        if (output != input)
            std::memcpy(output, input, (size_t)validIn * sizeof(float));
        return;
    }

    // 1. Zero-pad input block → FFT → FDL
    // Only copy validIn samples so partial blocks from variable-blocksize hosts
    // (e.g. FL Studio) don't read garbage past the end of the caller's buffer.
    std::memcpy(tempBuf_.data(), input, (size_t)validIn * sizeof(float));
    std::fill(tempBuf_.begin() + validIn, tempBuf_.end(), 0.0f);
    fft_->performRealOnlyForwardTransform(tempBuf_.data());
    std::copy(tempBuf_.begin(), tempBuf_.end(), fdl_[fdlHead_].begin());

    // 2. Head partitions
    std::fill(accumBuf_.begin(), accumBuf_.end(), 0.0f);

    const int headCount = std::min(kHeadPartitions, numPartitions_);
    for (int p = 0; p < headCount; ++p)
    {
        const int slot = (fdlHead_ - p + fdlDepth_) % fdlDepth_;
        complexMultiplyAdd(accumBuf_.data(), fdl_[slot].data(), H_[p].data(), fftSize_);
    }

    // 3. Tail partitions (cyclic)
    const int numTail = numPartitions_ - headCount;
    if (numTail > 0)
    {
        const int tailIter = std::min(kTailPerBlock, numTail);
        for (int i = 0; i < tailIter; ++i)
        {
            const int p    = headCount + tailCycleIdx_;
            const int slot = (fdlHead_ - p + fdlDepth_) % fdlDepth_;
            complexMultiplyAdd(accumBuf_.data(), fdl_[slot].data(), H_[p].data(), fftSize_);
            tailCycleIdx_ = (tailCycleIdx_ + 1) % numTail;
        }
    }

    // 4. IFFT — output lands in accumBuf_[0..fftSize_-1], normalised by 1/fftSize_ automatically
    fft_->performRealOnlyInverseTransform(accumBuf_.data());

    // 5. Overlap-add
    for (int n = 0; n < blockSize_; ++n)
        output[n] = accumBuf_[n] + overlapBuf_[n];

    std::memcpy(overlapBuf_.data(), accumBuf_.data() + blockSize_,
                (size_t)blockSize_ * sizeof(float));

    // 6. Advance FDL head
    fdlHead_ = (fdlHead_ + 1) % fdlDepth_;
}

// ---------------------------------------------------------------------------
// complexMultiplyAdd
//
// JUCE forward FFT output layout (N = fftSize_), buffer is 2*N floats:
//   [0],[1]       = DC bin       (re, im)
//   [2k],[2k+1]   = bin k        (re, im)  k = 1 .. N/2-1
//   [N],[N+1]     = Nyquist bin  (re, im)
//
// Only bins 0..N/2 need to be multiplied — that is all the IFFT reads.
// Indices N+2..2N-1 are conjugate mirrors we ignore.
//
// acc += a * b  (complex multiply-accumulate)
// ---------------------------------------------------------------------------

void NonUniformConvolver::complexMultiplyAdd(float*       acc,
                                             const float* a,
                                             const float* b,
                                             int          N) noexcept
{
    // DC [0,1]
    acc[0] += a[0] * b[0] - a[1] * b[1];
    acc[1] += a[0] * b[1] + a[1] * b[0];

    // Bins 1..N/2-1 at interleaved pairs [2..N-1]
    int i = 2;

#if NUC_HAS_AVX
    for (; i <= N - 8; i += 8)
    {
        __m256 av  = _mm256_loadu_ps(a   + i);
        __m256 bv  = _mm256_loadu_ps(b   + i);
        __m256 dv  = _mm256_loadu_ps(acc + i);

        __m256 a_re   = _mm256_moveldup_ps(av);
        __m256 a_im   = _mm256_movehdup_ps(av);
        __m256 b_swap = _mm256_shuffle_ps(bv, bv, 0xB1);

        __m256 prod1  = _mm256_mul_ps(a_re, bv);
        __m256 prod2  = _mm256_mul_ps(a_im, b_swap);
        __m256 cmul   = _mm256_addsub_ps(prod1, prod2);

        _mm256_storeu_ps(acc + i, _mm256_add_ps(dv, cmul));
    }
#endif

    for (; i < N; i += 2)
    {
        const float ar = a[i],     ai = a[i + 1];
        const float br = b[i],     bi = b[i + 1];
        acc[i]     += ar * br - ai * bi;
        acc[i + 1] += ar * bi + ai * br;
    }

    // Nyquist [N, N+1]
    acc[N]     += a[N] * b[N]     - a[N + 1] * b[N + 1];
    acc[N + 1] += a[N] * b[N + 1] + a[N + 1] * b[N];
}