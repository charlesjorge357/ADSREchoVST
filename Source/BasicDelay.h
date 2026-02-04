// BasicDelay.h - Stereo delay with BPM sync, ping pong, panning, and filters

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>
#endif

#include "CustomDelays.h"

class BasicDelay
{
public:
    enum class DelayMode { Normal, PingPong, Inverted };

    BasicDelay();
    ~BasicDelay();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    void setDelayTime(float delayMs);
    void setFeedback(float feedback);
    void setMix(float mix);
    void setMode(DelayMode mode);
    void setPan(float pan);
    void setLowpassFreq(float freq);
    void setHighpassFreq(float freq);

private:
    DelayLineWithSampleAccess<float> delayLineL { 88200 };  // ~2 sec at 44.1k
    DelayLineWithSampleAccess<float> delayLineR { 88200 };

    float delayTimeMs = 250.0f;
    float delayTimeSamples = 0.0f;
    float feedbackAmount = 0.3f;
    float mixAmount = 0.5f;

    DelayMode delayMode = DelayMode::Normal;
    float panValue = 0.0f;
    float lowpassFreqValue = 20000.0f;
    float highpassFreqValue = 20.0f;

    float sampleRate = 44100.0f;

    // Feedback state
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    // Feedback path filters (per-channel mono instances)
    juce::dsp::FirstOrderTPTFilter<float> lowpassL, lowpassR;
    juce::dsp::FirstOrderTPTFilter<float> highpassL, highpassR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicDelay)
};
