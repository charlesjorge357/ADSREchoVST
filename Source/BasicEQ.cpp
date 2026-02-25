#include "BasicEQ.h"

BasicEQ::BasicEQ() {}
BasicEQ::~BasicEQ() {}

void BasicEQ::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    lowShelf.prepare(spec);
    midPeak.prepare(spec);
    highShelf.prepare(spec);

    updateLowCoeffs();
    updateMidCoeffs();
    updateHighCoeffs();

    reset();
}

void BasicEQ::processBlock(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);

    lowShelf.process(ctx);
    midPeak.process(ctx);
    highShelf.process(ctx);
}

void BasicEQ::reset()
{
    lowShelf.reset();
    midPeak.reset();
    highShelf.reset();
}

// -------------------------------------------------------------------------
// Low Shelf
// -------------------------------------------------------------------------

void BasicEQ::setLowFreq(float freq)
{
    float clamped = juce::jlimit(20.0f, 500.0f, freq);
    if (lowFreq != clamped) { lowFreq = clamped; updateLowCoeffs(); }
}

void BasicEQ::setLowGain(float gainDb)
{
    float clamped = juce::jlimit(-24.0f, 24.0f, gainDb);
    if (lowGain != clamped) { lowGain = clamped; updateLowCoeffs(); }
}

void BasicEQ::setLowQ(float q)
{
    float clamped = juce::jlimit(0.1f, 10.0f, q);
    if (lowQ != clamped) { lowQ = clamped; updateLowCoeffs(); }
}

// -------------------------------------------------------------------------
// Mid Peak
// -------------------------------------------------------------------------

void BasicEQ::setMidFreq(float freq)
{
    float clamped = juce::jlimit(200.0f, 8000.0f, freq);
    if (midFreq != clamped) { midFreq = clamped; updateMidCoeffs(); }
}

void BasicEQ::setMidGain(float gainDb)
{
    float clamped = juce::jlimit(-24.0f, 24.0f, gainDb);
    if (midGain != clamped) { midGain = clamped; updateMidCoeffs(); }
}

void BasicEQ::setMidQ(float q)
{
    float clamped = juce::jlimit(0.1f, 10.0f, q);
    if (midQ != clamped) { midQ = clamped; updateMidCoeffs(); }
}

// -------------------------------------------------------------------------
// High Shelf
// -------------------------------------------------------------------------

void BasicEQ::setHighFreq(float freq)
{
    float clamped = juce::jlimit(2000.0f, 20000.0f, freq);
    if (highFreq != clamped) { highFreq = clamped; updateHighCoeffs(); }
}

void BasicEQ::setHighGain(float gainDb)
{
    float clamped = juce::jlimit(-24.0f, 24.0f, gainDb);
    if (highGain != clamped) { highGain = clamped; updateHighCoeffs(); }
}

void BasicEQ::setHighQ(float q)
{
    float clamped = juce::jlimit(0.1f, 10.0f, q);
    if (highQ != clamped) { highQ = clamped; updateHighCoeffs(); }
}

// -------------------------------------------------------------------------
// Coefficient helpers
// -------------------------------------------------------------------------

void BasicEQ::updateLowCoeffs()
{
    if (sampleRate <= 0.0) return;
    *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, lowFreq, lowQ, juce::Decibels::decibelsToGain(lowGain));
}

void BasicEQ::updateMidCoeffs()
{
    if (sampleRate <= 0.0) return;
    *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));
}

void BasicEQ::updateHighCoeffs()
{
    if (sampleRate <= 0.0) return;
    *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, highFreq, highQ, juce::Decibels::decibelsToGain(highGain));
}