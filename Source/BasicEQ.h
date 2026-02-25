// BasicEQ.h - 3-Band Parametric EQ (Low Shelf, Mid Peak, High Shelf)

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>
#endif

class BasicEQ
{
public:
    BasicEQ();
    ~BasicEQ();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    // Low shelf
    void setLowFreq(float freq);       // Hz, e.g. 20–500
    void setLowGain(float gainDb);     // dB
    void setLowQ(float q);             // Q factor

    // Mid peak
    void setMidFreq(float freq);       // Hz, e.g. 200–8000
    void setMidGain(float gainDb);     // dB
    void setMidQ(float q);             // Q factor

    // High shelf
    void setHighFreq(float freq);      // Hz, e.g. 2000–20000
    void setHighGain(float gainDb);    // dB
    void setHighQ(float q);            // Q factor

private:
    // Each band is a stereo IIR filter chain (L + R processed separately via ProcessorDuplicator)
    using MonoFilter = juce::dsp::IIR::Filter<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<MonoFilter, juce::dsp::IIR::Coefficients<float>>;

    StereoFilter lowShelf;
    StereoFilter midPeak;
    StereoFilter highShelf;

    // Parameter cache
    float lowFreq  = 200.0f;
    float lowGain  = 0.0f;
    float lowQ     = 0.707f;

    float midFreq  = 1000.0f;
    float midGain  = 0.0f;
    float midQ     = 0.707f;

    float highFreq = 8000.0f;
    float highGain = 0.0f;
    float highQ    = 0.707f;

    double sampleRate = 44100.0;

    void updateLowCoeffs();
    void updateMidCoeffs();
    void updateHighCoeffs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicEQ)
};
