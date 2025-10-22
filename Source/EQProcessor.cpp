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

    filterChainLeft.prepare(spec);
    filterChainRight.prepare(spec);

    // Initialize filter coefficients
    updateAllFilters();
}

void EQProcessor::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled)
        return;

    auto leftBlock = juce::dsp::AudioBlock<float>(buffer).getSingleChannelBlock(0);
    auto rightBlock = juce::dsp::AudioBlock<float>(buffer).getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    filterChainLeft.process(leftContext);
    filterChainRight.process(rightContext);
}

void EQProcessor::reset()
{
    filterChainLeft.reset();
    filterChainRight.reset();
}

void EQProcessor::setEnabled(bool shouldBeEnabled)
{
    enabled = shouldBeEnabled;
}

void EQProcessor::setLowFreq(float freq)
{
    lowFreq = juce::jlimit(20.0f, 500.0f, freq);
    updateLowFilter();
}

void EQProcessor::setLowGain(float gainDb)
{
    lowGain = juce::jlimit(-24.0f, 24.0f, gainDb);
    updateLowFilter();
}

void EQProcessor::setMidFreq(float freq)
{
    midFreq = juce::jlimit(200.0f, 5000.0f, freq);
    updateMidFilter();
}

void EQProcessor::setMidGain(float gainDb)
{
    midGain = juce::jlimit(-24.0f, 24.0f, gainDb);
    updateMidFilter();
}

void EQProcessor::setMidQ(float q)
{
    midQ = juce::jlimit(0.1f, 10.0f, q);
    updateMidFilter();
}

void EQProcessor::setHighFreq(float freq)
{
    highFreq = juce::jlimit(2000.0f, 20000.0f, freq);
    updateHighFilter();
}

void EQProcessor::setHighGain(float gainDb)
{
    highGain = juce::jlimit(-24.0f, 24.0f, gainDb);
    updateHighFilter();
}

void EQProcessor::updateLowFilter()
{
    auto& lowFilter = filterChainLeft.get<ChainPositions::LowBand>();
    auto& lowFilterR = filterChainRight.get<ChainPositions::LowBand>();

    *lowFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, lowFreq, 0.707f, juce::Decibels::decibelsToGain(lowGain));
    *lowFilterR.state = *lowFilter.state;
}

void EQProcessor::updateMidFilter()
{
    auto& midFilter = filterChainLeft.get<ChainPositions::MidBand>();
    auto& midFilterR = filterChainRight.get<ChainPositions::MidBand>();

    *midFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));
    *midFilterR.state = *midFilter.state;
}

void EQProcessor::updateHighFilter()
{
    auto& highFilter = filterChainLeft.get<ChainPositions::HighBand>();
    auto& highFilterR = filterChainRight.get<ChainPositions::HighBand>();

    *highFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, highFreq, 0.707f, juce::Decibels::decibelsToGain(highGain));
    *highFilterR.state = *highFilter.state;
}

void EQProcessor::updateAllFilters()
{
    updateLowFilter();
    updateMidFilter();
    updateHighFilter();
}

void EQProcessor::getMagnitudeForFrequency(double frequency, double& magnitude) const
{
    // TODO: Calculate combined frequency response of all filters
    juce::ignoreUnused(frequency, magnitude);
    magnitude = 1.0;
}
