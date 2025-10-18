/*
  ==============================================================================

    CompressorProcessor.h
    Dynamic Range Compressor Module

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class CompressorProcessor
{
public:
    CompressorProcessor();
    ~CompressorProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameter setters
    void setEnabled(bool enabled);
    void setThreshold(float thresholdDb);
    void setRatio(float ratio);
    void setAttack(float attackMs);
    void setRelease(float releaseMs);
    void setKnee(float kneeDb);
    void setMakeupGain(float gainDb);

    // Metering
    float getCurrentGainReduction() const { return gainReductionDb; }
    float getInputLevel() const { return inputLevelDb; }
    float getOutputLevel() const { return outputLevelDb; }

private:
    // JUCE's compressor
    juce::dsp::Compressor<float> compressor;

    // Level detection
    juce::dsp::BallisticsFilter<float> envelopeFollower;

    // Parameters
    bool enabled = false;
    float threshold = 0.0f;
    float ratio = 4.0f;
    float attack = 10.0f;
    float release = 100.0f;
    float knee = 0.0f;
    float makeupGain = 0.0f;

    // Metering
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<float> inputLevelDb { -100.0f };
    std::atomic<float> outputLevelDb { -100.0f };

    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProcessor)
};
