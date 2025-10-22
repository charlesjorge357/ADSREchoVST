/*
  ==============================================================================

    EQProcessor.cpp
    3-Band Parametric Equalizer Module

  ==============================================================================
*/

#include "EQProcessor.h"

EQProcessor::EQProcessor()
{
}

EQProcessor::~EQProcessor()
{
}

void EQProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    lowBandFilter.prepare(spec);
    midBandFilter.prepare(spec);
    highBandFilter.prepare(spec);

    // Set default filter parameters
    updateFilters();
}

void EQProcessor::process(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    lowBandFilter.process(context);
    midBandFilter.process(context);
    highBandFilter.process(context);
}

void EQProcessor::reset()
{
    lowBandFilter.reset();
    midBandFilter.reset();
    highBandFilter.reset();
}

void EQProcessor::setLowBand(float frequency, float gain)
{
    lowFreq = juce::jlimit(20.0f, 500.0f, frequency);
    lowGain = juce::jlimit(-24.0f, 24.0f, gain);
    updateFilters();
}

void EQProcessor::setMidBand(float frequency, float gain, float q)
{
    midFreq = juce::jlimit(200.0f, 5000.0f, frequency);
    midGain = juce::jlimit(-24.0f, 24.0f, gain);
    midQ = juce::jlimit(0.1f, 10.0f, q);
    updateFilters();
}

void EQProcessor::setHighBand(float frequency, float gain)
{
    highFreq = juce::jlimit(2000.0f, 20000.0f, frequency);
    highGain = juce::jlimit(-24.0f, 24.0f, gain);
    updateFilters();
}

void EQProcessor::updateFilters()
{
    // Low shelf
    *lowBandFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, lowFreq, 0.707f, juce::Decibels::decibelsToGain(lowGain));

    // Mid peak
    *midBandFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));

    // High shelf
    *highBandFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, highFreq, 0.707f, juce::Decibels::decibelsToGain(highGain));
}
