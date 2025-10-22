/*
  ==============================================================================

    ConvolutionProcessor.h
    Convolution Reverb Module using impulse responses

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class ConvolutionProcessor
{
public:
    ConvolutionProcessor();
    ~ConvolutionProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameter setters
    void setEnabled(bool enabled);
    void setMix(float mix);
    void setPreDelay(float preDelayMs);
    void setStretch(float stretchFactor);

    // IR Management
    void loadImpulseResponse(const juce::File& irFile);
    void loadImpulseResponse(const void* sourceData, size_t sourceDataSize);
    void setIRIndex(int index); // For preset IRs
    juce::StringArray getAvailableIRs() const;

private:
    // JUCE's built-in convolution engine
    juce::dsp::Convolution convolutionL;
    juce::dsp::Convolution convolutionR;

    // Pre-delay buffer
    juce::dsp::DelayLine<float> preDelayLine;

    // IR storage
    juce::AudioBuffer<float> currentIR;
    juce::StringArray irNames;
    int currentIRIndex = 0;

    // Parameters
    bool enabled = false;
    float mix = 0.5f;
    float preDelayMs = 0.0f;
    float stretchFactor = 1.0f;

    double sampleRate = 44100.0;

    // Helper methods
    void loadBuiltInIRs();
    void applyIRStretch();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionProcessor)
};
