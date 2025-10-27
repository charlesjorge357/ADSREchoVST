/*
  ==============================================================================

    EQProcessor.h
    3-Band Parametric Equalizer Module

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class EQProcessor
{
public:
    EQProcessor();
    ~EQProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameter setters
    void setEnabled(bool enabled);

    // Low shelf
    void setLowFreq(float freq);
    void setLowGain(float gainDb);

    // Mid peak/notch
    void setMidFreq(float freq);
    void setMidGain(float gainDb);
    void setMidQ(float q);

    // High shelf
    void setHighFreq(float freq);
    void setHighGain(float gainDb);

    // Get frequency response for GUI visualization
    void getMagnitudeForFrequency(double frequency, double& magnitude) const;

private:
    // Filter chains (stereo)
    using FilterType = juce::dsp::IIR::Filter<float>;
    using FilterChain = juce::dsp::ProcessorChain<FilterType, FilterType, FilterType>;

    FilterChain filterChainLeft;
    FilterChain filterChainRight;

    enum ChainPositions
    {
        LowBand,
        MidBand,
        HighBand
    };

    // Parameters
    bool enabled = false;

    float lowFreq = 80.0f;
    float lowGain = 0.0f;

    float midFreq = 1000.0f;
    float midGain = 0.0f;
    float midQ = 0.707f;

    float highFreq = 8000.0f;
    float highGain = 0.0f;

    double sampleRate = 44100.0;

    // Helper methods
    void updateLowFilter();
    void updateMidFilter();
    void updateHighFilter();
    void updateAllFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQProcessor)
};
