/*
  ==============================================================================

    DelayProcessor.h
    Advanced Stereo Delay Module
    Features: Haas effect, ping-pong, distortion, tempo sync

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "CustomDelays.h"

class DelayProcessor
{
public:
    enum class StereoMode
    {
        Stereo,      // Independent L/R delays
        PingPong,    // Alternating L/R feedback
        Haas         // Haas effect (short delay for width)
    };

    enum class SyncDivision
    {
        // Note divisions for tempo sync
        Whole,          // 1/1
        Half,           // 1/2
        Quarter,        // 1/4
        Eighth,         // 1/8
        Sixteenth,      // 1/16
        DottedHalf,     // 1/2.
        DottedQuarter,  // 1/4.
        DottedEighth,   // 1/8.
        TripletHalf,    // 1/2T
        TripletQuarter, // 1/4T
        TripletEighth   // 1/8T
    };

    DelayProcessor();
    ~DelayProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameter setters
    void setDelayTimeLeft(float timeMs);
    void setDelayTimeRight(float timeMs);
    void setFeedback(float feedback);
    void setMix(float mix);
    void setStereoMode(StereoMode mode);
    void setSyncEnabled(bool enabled);
    void setSyncDivisionLeft(SyncDivision division);
    void setSyncDivisionRight(SyncDivision division);
    void setLowCutFreq(float freq);
    void setHighCutFreq(float freq);
    void setDistortion(float amount);
    void setDistortionMix(float mix);
    void setTempo(double bpm);

    // Haas effect specific
    void setHaasDelayMs(float ms);

private:
    // Delay lines
    DelayLineWithSampleAccess<float> delayLineLeft;
    DelayLineWithSampleAccess<float> delayLineRight;
    DelayLineWithSampleAccess<float> haasDelayLine; // For Haas effect

    // Feedback filters (to prevent buildup)
    juce::dsp::IIR::Filter<float> lowCutFilterL, lowCutFilterR;
    juce::dsp::IIR::Filter<float> highCutFilterL, highCutFilterR;

    // Distortion path
    juce::dsp::WaveShaper<float> distortionL, distortionR;
    std::function<float(float)> distortionFunction;

    // Parameters
    StereoMode stereoMode = StereoMode::Stereo;
    float delayTimeLeftMs = 500.0f;
    float delayTimeRightMs = 500.0f;
    float feedback = 0.3f;
    float mix = 0.5f;
    bool syncEnabled = false;
    SyncDivision syncDivLeft = SyncDivision::Quarter;
    SyncDivision syncDivRight = SyncDivision::Quarter;
    float lowCutFreq = 20.0f;
    float highCutFreq = 20000.0f;
    float distortionAmount = 0.0f;
    float distortionMix = 0.0f;
    double tempo = 120.0;

    // Haas effect
    float haasDelayMs = 15.0f; // Typical Haas delay (5-40ms)

    double sampleRate = 44100.0;

    // Helper methods
    int msToSamples(float ms) const;
    float syncedDelayTime(SyncDivision division) const;
    void processStereoMode(juce::AudioBuffer<float>& buffer);
    void processPingPongMode(juce::AudioBuffer<float>& buffer);
    void processHaasMode(juce::AudioBuffer<float>& buffer);
    void applyDistortion(float& sample);
    void updateDistortionFunction();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayProcessor)
};
