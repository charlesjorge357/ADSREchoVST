/*
  ==============================================================================

    ReverbProcessor.h
    Algorithmic Reverb Module (Plate and Hall models)

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "DatorroHall.h"

class ReverbProcessor
{
public:
    enum class ReverbType
    {
        Hall,
        Plate
    };

    ReverbProcessor();
    ~ReverbProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameter setters
    void setReverbType(ReverbType type);
    void setSize(float size);
    void setDiffusion(float diffusion);
    void setDecay(float decayTime);
    void setMix(float mix);
    void setPreDelay(float preDelayMs);

    ReverbType getCurrentType() const { return currentType; }

private:
    // Reverb algorithm instances
    std::unique_ptr<DatorroHall> hallReverbL;
    std::unique_ptr<DatorroHall> hallReverbR;

    // Plate reverb (will use different topology)
    juce::dsp::Reverb plateReverb;

    // Pre-delay buffer
    juce::dsp::DelayLine<float> preDelayLine;

    // All-pass diffusers for plate character
    std::vector<Allpass<float>> plateDiffusers;

    // Parameters
    ReverbType currentType = ReverbType::Plate;
    float size = 0.5f;
    float diffusion = 0.7f;
    float decay = 0.5f;
    float mix = 0.3f;
    float preDelayMs = 0.0f;

    double sampleRate = 44100.0;

    // Helper methods
    void processHall(juce::AudioBuffer<float>& buffer);
    void processPlate(juce::AudioBuffer<float>& buffer);
    void updatePlateParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbProcessor)
};
