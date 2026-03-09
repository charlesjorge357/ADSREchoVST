#pragma once

#include <vector>
#include <memory>

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_dsp/juce_dsp.h>
#endif

// Mono, partitioned overlap-add convolution with cyclic tail deferral.
//
// Usage:
//   prepare(sampleRate, blockSize);       // blockSize must be power of two
//   loadImpulseResponse(monoIR);
//   processBlock(input, output, blockSize);
//
// NOTE: output must NOT alias input.

class NonUniformConvolver
{
public:
    NonUniformConvolver()  = default;
    ~NonUniformConvolver() = default;

    NonUniformConvolver(const NonUniformConvolver&)            = delete;
    NonUniformConvolver& operator=(const NonUniformConvolver&) = delete;

    void prepare(double sampleRate, int blockSize);
    void loadImpulseResponse(const std::vector<float>& ir);
    void reset();

    // output must NOT alias input.
    void processBlock(const float* input, float* output, int numSamples);

    bool isPrepared()   const noexcept { return blockSize_ > 0; }
    bool hasIR()        const noexcept { return numPartitions_ > 0; }
    int  getBlockSize() const noexcept { return blockSize_; }

private:
    static void complexMultiplyAdd(float* acc, const float* a, const float* b, int fftSize) noexcept;

    static constexpr int kHeadPartitions = 4;
    static constexpr int kTailPerBlock   = 4;

    int blockSize_ = 0;
    int fftSize_   = 0;
    int fftOrder_  = 0;

    std::unique_ptr<juce::dsp::FFT> fft_;

    std::vector<std::vector<float>> H_;      // IR partitions, each 2*fftSize_ floats
    int numPartitions_ = 0;

    std::vector<std::vector<float>> fdl_;    // FDL ring buffer, each 2*fftSize_ floats
    int fdlHead_  = 0;
    int fdlDepth_ = 0;

    std::vector<float> overlapBuf_;          // blockSize_ floats — OLA tail
    std::vector<float> accumBuf_;            // 2*fftSize_ floats
    std::vector<float> tempBuf_;             // 2*fftSize_ floats

    int tailCycleIdx_ = 0;
};