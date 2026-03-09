#pragma once

// ---------------------------------------------------------------------------
// Engine selection — flip to 0 to use JUCE's built-in convolver instead.
// ---------------------------------------------------------------------------
#define USE_CUSTOM_CONVOLVER 1

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_audio_formats/juce_audio_formats.h>
  #include <juce_dsp/juce_dsp.h>
#endif

#if USE_CUSTOM_CONVOLVER
  #include "fftconvolver/TwoStageFFTConvolver.h"
#endif

// Forward declaration
class IRBank;

// Parameters for the convolution reverb engine
struct ConvolutionParameters
{
    float mix        = 0.5f;     // 0 = fully dry, 1 = fully wet
    float preDelay   = 0.0f;     // pre delay before IR, in ms

    int   irIndex    = 0;        // which IR to use (0, 1, 2, ...)
    float irGainDb   = 0.0f;     // gain applied to IR output

    float lowCutHz   = 80.0f;    // high pass cutoff
    float highCutHz  = 12000.0f; // low pass cutoff
};

// Stereo convolution reverb wrapper.
// Engine selected at compile time via USE_CUSTOM_CONVOLVER.
class Convolution
{
public:
    Convolution();
    ~Convolution();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters(const ConvolutionParameters& newParams);
    ConvolutionParameters& getParameters();

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    void loadIR(const juce::File& file);
    void loadIRFromMemory(const void* data,
                          size_t dataSize,
                          double sampleRate,
                          int numChannels);

    void setIRBank(std::shared_ptr<IRBank> bank);
    void loadIRAtIndex(int index);

    void loadCustomIR(const juce::File& file);
    void clearCustomIR();
    bool hasCustomIR()          const { return customIRActive; }
    juce::String getCustomIRPath() const { return customIRPath; }

    // Force reload regardless of cached index (call after preset restore)
    void forceLoadIRAtIndex(int index);

    // True if the last requested bank IR was missing/out-of-range
    bool isIRMissing() const { return irMissingFlag; }

private:
    void updateFilters();
    void updatePreDelay();

#if USE_CUSTOM_CONVOLVER
    // Reads a stereo (or mono duplicated to stereo) IR from a file,
    // resampling to targetSampleRate if the file's native rate differs.
    using StereoIR = std::pair<std::vector<float>, std::vector<float>>;
    static StereoIR readStereoIR(const juce::File& file, double targetSampleRate);
    static StereoIR readStereoIRFromMemory(const void* data, size_t dataSize, double targetSampleRate);
    void loadCustomEngineIR(const std::vector<float>& irL, const std::vector<float>& irR);
#endif

    ConvolutionParameters parameters;

    bool   prepared          = false;
    double currentSampleRate = 44100.0;
    float  preDelaySamples   = 0.0f;
    bool   isPreDelayActive  = false;
    int    currentIRIndex    = -1;
    bool   customIRActive    = false;
    juce::String customIRPath;
    bool   irMissingFlag     = false;

    // Cached filter frequencies - avoids recomputing coefficients when unchanged
    float lastLowCutHz  = -1.0f;
    float lastHighCutHz = -1.0f;

    std::shared_ptr<IRBank> irBank;

#if USE_CUSTOM_CONVOLVER
    // Two mono convolvers — L and R convolved separately to preserve stereo IRs.
    fftconvolver::TwoStageFFTConvolver convolverL;
    fftconvolver::TwoStageFFTConvolver convolverR;
    size_t headBlockSize_ = 512;
    size_t tailBlockSize_ = 4096;
    // Scratch buffer: TwoStageFFTConvolver requires separate input/output pointers.
    // After the head convolver writes to output, the tail reads from "input" — if
    // input==output, the tail gets the head's result instead of the dry signal,
    // causing runaway feedback for IRs longer than tailBlockSize samples.
    std::vector<float> monoInBuf;
#else
    juce::dsp::Convolution convolver { juce::dsp::Convolution::NonUniform { 8192 } };
#endif

    static constexpr int kMaxPreDelaySeconds = 2;
    static constexpr int kMaxSampleRate      = 192000;
    static constexpr int kMaxDelaySamples    = kMaxPreDelaySeconds * kMaxSampleRate;

    // None interpolation: pre-delay is always a static value, no sub-sample
    // accuracy needed, avoids a multiply-add per sample vs Linear
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> preDelayL { kMaxDelaySamples };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> preDelayR { kMaxDelaySamples };

    // Two stereo filters instead of four mono filters - halves filter overhead
    // and avoids branching on numChannels in processBlock
    using MonoFilter   = juce::dsp::IIR::Filter<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<MonoFilter, juce::dsp::IIR::Coefficients<float>>;

    StereoFilter lowCut;
    StereoFilter highCut;

    juce::dsp::DryWetMixer<float> dryWetMixer;

    // Smoothed IR gain - setTargetValue is called only when irGainDb changes,
    // not every block
    juce::SmoothedValue<float> smoothedIRGain;
};
