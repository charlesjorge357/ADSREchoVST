/*
  ==============================================================================

    CompressorProcessor.cpp
    Dynamic Range Compressor Module

  ==============================================================================
*/

#include "CompressorProcessor.h"

CompressorProcessor::CompressorProcessor()
{
}

CompressorProcessor::~CompressorProcessor()
{
}

void CompressorProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    compressor.prepare(spec);

    // Set default parameters
    compressor.setThreshold(threshold);
    compressor.setRatio(ratio);
    compressor.setAttack(attack);
    compressor.setRelease(release);
}

void CompressorProcessor::process(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
}

void CompressorProcessor::reset()
{
    compressor.reset();
}

void CompressorProcessor::setThreshold(float newThreshold)
{
    threshold = newThreshold;
    compressor.setThreshold(threshold);
}

void CompressorProcessor::setRatio(float newRatio)
{
    ratio = juce::jlimit(1.0f, 20.0f, newRatio);
    compressor.setRatio(ratio);
}

void CompressorProcessor::setAttack(float newAttack)
{
    attack = juce::jlimit(0.1f, 100.0f, newAttack);
    compressor.setAttack(attack);
}

void CompressorProcessor::setRelease(float newRelease)
{
    release = juce::jlimit(10.0f, 1000.0f, newRelease);
    compressor.setRelease(release);
}
