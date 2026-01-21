#pragma once

#include <JuceHeader.h>

// Forward declaration
class IRBank;

// Parameters for the convolution reverb engine
struct ConvolutionParameters
{
    float mix        = 0.5f;    // 0 = fully dry, 1 = fully wet
    float preDelay   = 0.0f;    // pre delay before IR, in ms

    float irIndex    = 0.0f;    // which IR to use (0, 1, 2, ...)
    float irGainDb   = 0.0f;    // gain applied to IR output

    float lowCutHz   = 80.0f;   // high pass cutoff
    float highCutHz  = 12000.0f; // low pass cutoff
};

// Simple stereo convolution reverb wrapper using juce::dsp::Convolution
class Convolution
{
public:
    Convolution();
    ~Convolution();

    // Prepare internal DSP for a given sample rate, block size, and channel count
    void prepare(const juce::dsp::ProcessSpec& spec);

    // Reset internal state (clear delay lines, filters, convolver history)
    void reset();

    // Set all parameters at once (called by ConvolutionModule)
    void setParameters(const ConvolutionParameters& newParams);

    // Get current parameters (for inspection or smoothing)
    ConvolutionParameters& getParameters();

    // Main processing entry point
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // IR loading helpers
    void loadIR(const juce::File& file);
    void loadIRFromMemory(const void* data,
                         size_t dataSize,
                         double sampleRate,
                         int numChannels);
    
    // IR bank management
    void setIRBank(std::shared_ptr<IRBank> bank);
    void loadIRAtIndex(int index);

private:
    // Helper: configure HP/LP filters for a given sample rate
    void updateFilters();
    
    // Helper: update pre-delay samples based on parameters
    void updatePreDelay();

    ConvolutionParameters parameters;

    bool prepared = false;
    double currentSampleRate = 44100.0;
    float preDelaySamples = 0.0f;
    int currentIRIndex = -1;
    
    std::shared_ptr<IRBank> irBank;

    // JUCE convolution engine (handles stereo buffers if IR is stereo)
    juce::dsp::Convolution convolver;

    // Simple stereo pre-delay using dsp::DelayLine
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL { 48000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayR { 48000 };

    // Simple HP / LP filters per channel for tone shaping
    juce::dsp::IIR::Filter<float> lowCutL;
    juce::dsp::IIR::Filter<float> lowCutR;
    juce::dsp::IIR::Filter<float> highCutL;
    juce::dsp::IIR::Filter<float> highCutR;
    
    // Dry buffer for dry/wet mixing
    juce::AudioBuffer<float> dryBuffer;
};