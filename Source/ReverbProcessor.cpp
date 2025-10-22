/*
  ==============================================================================

    ReverbProcessor.cpp
    Algorithmic Reverb Module (Plate and Hall models)

  ==============================================================================
*/

#include "ReverbProcessor.h"

ReverbProcessor::ReverbProcessor()
{
}

ReverbProcessor::~ReverbProcessor()
{
}

void ReverbProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Prepare plate reverb
    juce::dsp::Reverb::Parameters params;
    params.roomSize = size;
    params.damping = 0.5f;
    params.wetLevel = mix;
    params.dryLevel = 1.0f - mix;
    params.width = 1.0f;
    plateReverb.setParameters(params);
    plateReverb.prepare(spec);

    // TODO: Initialize hall reverb instances
    // hallReverbL = std::make_unique<DatorroHall>();
    // hallReverbR = std::make_unique<DatorroHall>();
}

void ReverbProcessor::process(juce::AudioBuffer<float>& buffer)
{
    if (currentType == ReverbType::Plate)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        plateReverb.process(context);
    }
    // TODO: Implement Hall processing
}

void ReverbProcessor::reset()
{
    plateReverb.reset();
}

void ReverbProcessor::setReverbType(ReverbType type)
{
    currentType = type;
}

void ReverbProcessor::setSize(float newSize)
{
    size = juce::jlimit(0.0f, 1.0f, newSize);
}

void ReverbProcessor::setDiffusion(float newDiffusion)
{
    diffusion = juce::jlimit(0.0f, 1.0f, newDiffusion);
}

void ReverbProcessor::setDecay(float decayTime)
{
    decay = juce::jlimit(0.0f, 1.0f, decayTime);
}

void ReverbProcessor::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);
}

void ReverbProcessor::setPreDelay(float preDelayMs)
{
    preDelayMs = juce::jlimit(0.0f, 500.0f, preDelayMs);
}
