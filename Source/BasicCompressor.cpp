// BasicCompressor.cpp
// Optical-style compressor inspired by LA-2A, SSL G-Bus, and Neve 33609.

#include "BasicCompressor.h"

BasicCompressor::BasicCompressor() {}
BasicCompressor::~BasicCompressor() {}

void BasicCompressor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Pre-detector low-mid emphasis filter
    // The LA-2A optical cell is most sensitive around 300-600 Hz.
    // A lowpass at 800 Hz on the detector signal biases compression
    // toward low-mid energy, which naturally de-muddies reverb tails.
    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    detectorFilterL.prepare(monoSpec);
    detectorFilterR.prepare(monoSpec);
    detectorFilterL.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    detectorFilterR.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    detectorFilterL.setCutoffFrequency(800.0f);
    detectorFilterR.setCutoffFrequency(800.0f);

    updateTimeCoefficients();
    reset();
}

void BasicCompressor::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* leftData  = buffer.getWritePointer(0);
    float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Pre-compute linear gains from dB params
    const float inputLinear  = juce::Decibels::decibelsToGain(inputGainDb);
    const float outputLinear = juce::Decibels::decibelsToGain(outputGainDb);

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- Input gain stage ----
        float inL = leftData[i] * inputLinear;
        float inR = (rightData != nullptr) ? rightData[i] * inputLinear : inL;

        // ---- Detector: low-mid filtered, linked RMS ----
        // Filter the detector signal to weight low-mid frequencies
        float detL = detectorFilterL.processSample(0, inL);
        float detR = (rightData != nullptr)
                        ? detectorFilterR.processSample(0, inR)
                        : detL;

        // Linked RMS: sum of squares, averaged, then sqrt
        double instantPower = 0.5 * ((double)detL * detL + (double)detR * detR);
        double instantRms   = std::sqrt(instantPower + 1e-18); // epsilon avoids log(0)

        // ---- Program-dependent envelope follower ----
        // LA-2A style: attack speeds up for louder signals,
        // release slows down for sustained content (SSL "auto" release feel)
        double inputDb = juce::Decibels::gainToDecibels((float)instantRms);

        double aCoeff = computeAttackCoeff(inputDb);
        double rCoeff = computeReleaseCoeff(inputDb);

        if (instantRms > envelopeState)
            envelopeState = aCoeff * envelopeState + (1.0 - aCoeff) * instantRms;
        else
            envelopeState = rCoeff * envelopeState + (1.0 - rCoeff) * instantRms;

        // ---- Gain computer (soft knee, Neve-style) ----
        double envDb = juce::Decibels::gainToDecibels((float)envelopeState);
        double targetGrDb = computeGainReductionDb(envDb);

        // ---- Smooth gain reduction (models optical cell lag) ----
        // A secondary smoother on the GR signal itself gives the
        // characteristic optical "rounding" — gain reduction never snaps
        gainReductionState = gainReductionState * 0.9995
                           + targetGrDb         * 0.0005;

        // For very fast transients above threshold, let GR move faster
        // (models the LED driving harder into the optical cell)
        if (targetGrDb < gainReductionState)
            gainReductionState = gainReductionState * 0.995
                               + targetGrDb         * 0.005;

        double grLinear = juce::Decibels::decibelsToGain((float)gainReductionState);

        // ---- Apply gain reduction + output gain ----
        leftData[i] = (float)(inL * grLinear) * outputLinear;

        if (rightData != nullptr)
            rightData[i] = (float)(inR * grLinear) * outputLinear;
    }
}

void BasicCompressor::reset()
{
    envelopeState      = 0.0;
    gainReductionState = 0.0;
    detectorFilterL.reset();
    detectorFilterR.reset();
}

// ---------------------------------------------------------------------------
// Soft-knee gain computer
// Below (threshold - knee/2): no compression
// Within knee: gradually blend in compression (Neve 33609 style smooth knee)
// Above (threshold + knee/2): full ratio compression
// ---------------------------------------------------------------------------
double BasicCompressor::computeGainReductionDb(double inputDb) const
{
    const double T  = (double)thresholdDb;
    const double R  = (double)ratio;
    const double kH = (double)kneeWidthDb * 0.5;

    // Below knee: unity
    if (inputDb < T - kH)
        return 0.0;

    // Within soft knee
    if (inputDb <= T + kH)
    {
        double x = inputDb - (T - kH);
        double kneeRange = kneeWidthDb;
        // Quadratic blend from 1:1 to full ratio across knee
        double blendedSlope = 1.0 + (R - 1.0) * (x / kneeRange) * (x / kneeRange);
        double outputDb = (T - kH) + x / blendedSlope;
        return outputDb - inputDb;
    }

    // Above knee: full ratio
    double outputDb = T + (inputDb - T) / R;
    return outputDb - inputDb;
}

// ---------------------------------------------------------------------------
// Program-dependent time constants — LA-2A behaviour
// Attack: faster when signal is well above threshold (LED drives harder)
// Release: slower for sustained signals, faster for brief transients
// ---------------------------------------------------------------------------
double BasicCompressor::computeAttackCoeff(double inputDb) const
{
    // How far above threshold is the signal?
    double overdrive = inputDb - (double)thresholdDb;
    overdrive = juce::jmax(0.0, overdrive);

    // At 0 dB overdrive: use nominal attack time
    // At 20 dB overdrive: attack is ~3x faster (more light hits the cell)
    double scaledAttackMs = (double)attackMs / (1.0 + overdrive * 0.1);
    scaledAttackMs = juce::jmax(0.1, scaledAttackMs);

    double attackSamples = (scaledAttackMs / 1000.0) * sampleRate;
    return std::exp(-1.0 / attackSamples);
}

double BasicCompressor::computeReleaseCoeff(double inputDb) const
{
    // SSL-style: if signal has been sustained above threshold, release slows
    // We approximate this by scaling release with envelope level
    double envelopeDb = juce::Decibels::gainToDecibels((float)envelopeState);
    double sustainAmount = juce::jmax(0.0, envelopeDb - (double)thresholdDb);

    // Up to 2x longer release for heavily compressed sustained signals
    double scaledReleaseMs = (double)releaseMs * (1.0 + sustainAmount * 0.05);
    scaledReleaseMs = juce::jmin(scaledReleaseMs, (double)releaseMs * 2.0);

    double releaseSamples = (scaledReleaseMs / 1000.0) * sampleRate;
    return std::exp(-1.0 / releaseSamples);
}

// ---------------------------------------------------------------------------
// Parameter setters — same guard pattern as BasicDelay/BasicEQ
// ---------------------------------------------------------------------------

void BasicCompressor::setThreshold(float dBFS)
{
    float clamped = juce::jlimit(-60.0f, 0.0f, dBFS);
    if (thresholdDb != clamped) thresholdDb = clamped;
}

void BasicCompressor::setRatio(float r)
{
    float clamped = juce::jlimit(1.0f, 20.0f, r);
    if (ratio != clamped) ratio = clamped;
}

void BasicCompressor::setAttack(float ms)
{
    float clamped = juce::jlimit(1.0f, 200.0f, ms);
    if (attackMs != clamped)
    {
        attackMs = clamped;
        updateTimeCoefficients();
    }
}

void BasicCompressor::setRelease(float ms)
{
    float clamped = juce::jlimit(10.0f, 2000.0f, ms);
    if (releaseMs != clamped)
    {
        releaseMs = clamped;
        updateTimeCoefficients();
    }
}

void BasicCompressor::setInputGain(float dB)
{
    float clamped = juce::jlimit(-18.0f, 18.0f, dB);
    if (inputGainDb != clamped) inputGainDb = clamped;
}

void BasicCompressor::setOutputGain(float dB)
{
    float clamped = juce::jlimit(-18.0f, 18.0f, dB);
    if (outputGainDb != clamped) outputGainDb = clamped;
}

void BasicCompressor::updateTimeCoefficients()
{
    if (sampleRate <= 0.0) return;
    double attackSamples  = (attackMs  / 1000.0) * sampleRate;
    double releaseSamples = (releaseMs / 1000.0) * sampleRate;
    attackCoeff  = std::exp(-1.0 / attackSamples);
    releaseCoeff = std::exp(-1.0 / releaseSamples);
}