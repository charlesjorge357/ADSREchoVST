// BasicDelay.h - Stereo delay with BPM sync, ping pong, panning, and filters
// Uses readFractional() for smooth continuous read-head movement during tempo automation

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

    enum class SyncDivision
    {
        Whole = 0,
        Half,
        Quarter,        // 1 beat
        Eighth,
        Sixteenth,
        DottedHalf,
        DottedQuarter,
        DottedEighth,
        TripletHalf,
        TripletQuarter,
        TripletEighth,
    };

    BasicDelay();
    ~BasicDelay();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    // Free-running ms delay (disables BPM sync)
    void setDelayTime(float delayMs);

    // Enable BPM sync and set the subdivision
    void setBpmSync(bool enabled, float bpm = 120.0f, SyncDivision division = SyncDivision::Quarter);

    // Call once per block from your processor when BPM sync is active:
    //   delay.setCurrentBpm(playHead->getPosition()->getBpm().orFallback(120.0));
    void setCurrentBpm(float bpm);

    void setFeedback(float feedback);
    void setMix(float mix);
    void setMode(DelayMode mode);
    void setPan(float pan);
    void setLowpassFreq(float freq);
    void setHighpassFreq(float freq);

    // How long the read head glides to a new delay time (default 50ms)
    void setRampTimeMs(float rampMs);

private:
    static float divisionToMs(float bpm, SyncDivision div) noexcept;
    void applyTargetDelayMs(float ms);

    // ~4 sec buffer at 48k -- enough headroom for slow BPM subdivisions
    DelayLineWithSampleAccess<float> delayLineL { 192001 };
    DelayLineWithSampleAccess<float> delayLineR { 192001 };

    float sampleRate = 44100.0f;

    // Per-sample smoother drives readFractional() directly -- no setDelay() in the hot path
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelaySamples;
    float rampTimeMs  = 50.0f;
    float delayTimeMs = 250.0f;

    // BPM sync state
    bool         bpmSyncEnabled = false;
    float        currentBpm     = 120.0f;
    SyncDivision syncDivision   = SyncDivision::Quarter;

    float feedbackAmount    = 0.3f;
    float mixAmount         = 0.5f;
    DelayMode delayMode     = DelayMode::Normal;
    float panValue          = 0.0f;
    float lowpassFreqValue  = 20000.0f;
    float highpassFreqValue = 20.0f;

    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    juce::dsp::FirstOrderTPTFilter<float> lowpassL,  lowpassR;
    juce::dsp::FirstOrderTPTFilter<float> highpassL, highpassR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicDelay)
};