/*
  ==============================================================================

    DelayProcessor.cpp
    Simple stereo delay implementation

  ==============================================================================
*/

#include "DelayProcessor.h"

DelayProcessor::DelayProcessor()
    : delayLineLeft(88200)  // Max 2 seconds at 44.1kHz
    , delayLineRight(88200)
    , haasDelayLine(4410)   // Max 100ms
{
}

DelayProcessor::~DelayProcessor()
{
}

void DelayProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Calculate max delay buffer size (2 seconds)
    int maxDelaySamples = static_cast<int>(sampleRate * 2.0);

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    delayLineLeft.setSize(1, maxDelaySamples);
    delayLineRight.setSize(1, maxDelaySamples);
    haasDelayLine.setSize(1, static_cast<int>(sampleRate * 0.1)); // 100ms max

    // Prepare filters
    lowCutFilterL.prepare(monoSpec);
    lowCutFilterR.prepare(monoSpec);
    highCutFilterL.prepare(monoSpec);
    highCutFilterR.prepare(monoSpec);

    reset();
}

void DelayProcessor::reset()
{
    delayLineLeft.reset();
    delayLineRight.reset();
    haasDelayLine.reset();

    lowCutFilterL.reset();
    lowCutFilterR.reset();
    highCutFilterL.reset();
    highCutFilterR.reset();
}

void DelayProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 2)
        return; // Need stereo

    // Get current delay times in samples
    int delayTimeLSamples = msToSamples(delayTimeLeftMs);
    int delayTimeRSamples = msToSamples(delayTimeRightMs);

    delayLineLeft.setDelay(delayTimeLSamples);
    delayLineRight.setDelay(delayTimeRSamples);

    // Process based on stereo mode
    switch (stereoMode)
    {
        case StereoMode::Stereo:
            processStereoMode(buffer);
            break;

        case StereoMode::PingPong:
            processPingPongMode(buffer);
            break;

        case StereoMode::Haas:
            processHaasMode(buffer);
            break;
    }
}

void DelayProcessor::processStereoMode(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Read delayed samples
        float delayedL = delayLineLeft.popSample(0);
        float delayedR = delayLineRight.popSample(0);

        // Get input samples
        float inputL = leftChannel[sample];
        float inputR = rightChannel[sample];

        // Write input + feedback to delay lines
        delayLineLeft.pushSample(0, inputL + delayedL * feedback);
        delayLineRight.pushSample(0, inputR + delayedR * feedback);

        // Mix dry and wet
        leftChannel[sample] = inputL * (1.0f - mix) + delayedL * mix;
        rightChannel[sample] = inputR * (1.0f - mix) + delayedR * mix;
    }
}

void DelayProcessor::processPingPongMode(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Read delayed samples
        float delayedL = delayLineLeft.popSample(0);
        float delayedR = delayLineRight.popSample(0);

        // Get input samples
        float inputL = leftChannel[sample];
        float inputR = rightChannel[sample];

        // Ping-pong: L feedback goes to R, R feedback goes to L
        delayLineLeft.pushSample(0, inputL + delayedR * feedback);
        delayLineRight.pushSample(0, inputR + delayedL * feedback);

        // Mix dry and wet
        leftChannel[sample] = inputL * (1.0f - mix) + delayedL * mix;
        rightChannel[sample] = inputR * (1.0f - mix) + delayedR * mix;
    }
}

void DelayProcessor::processHaasMode(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    int haasDelaySamples = msToSamples(haasDelayMs);
    haasDelayLine.setDelay(haasDelaySamples);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float input = (leftChannel[sample] + rightChannel[sample]) * 0.5f;

        // Get delayed signal
        float delayed = haasDelayLine.popSample(0);

        // Push input to delay
        haasDelayLine.pushSample(0, input);

        // Output: L = original, R = delayed (creates stereo width)
        leftChannel[sample] = input;
        rightChannel[sample] = delayed * mix + input * (1.0f - mix);
    }
}

// Parameter setters
void DelayProcessor::setDelayTimeLeft(float timeMs)
{
    delayTimeLeftMs = juce::jlimit(1.0f, 2000.0f, timeMs);
}

void DelayProcessor::setDelayTimeRight(float timeMs)
{
    delayTimeRightMs = juce::jlimit(1.0f, 2000.0f, timeMs);
}

void DelayProcessor::setFeedback(float newFeedback)
{
    feedback = juce::jlimit(0.0f, 0.95f, newFeedback);
}

void DelayProcessor::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);
}

void DelayProcessor::setStereoMode(StereoMode mode)
{
    stereoMode = mode;
}

void DelayProcessor::setSyncEnabled(bool enabled)
{
    syncEnabled = enabled;
}

void DelayProcessor::setSyncDivisionLeft(SyncDivision division)
{
    syncDivLeft = division;
}

void DelayProcessor::setSyncDivisionRight(SyncDivision division)
{
    syncDivRight = division;
}

void DelayProcessor::setLowCutFreq(float freq)
{
    lowCutFreq = freq;
    // TODO: Update filter coefficients
}

void DelayProcessor::setHighCutFreq(float freq)
{
    highCutFreq = freq;
    // TODO: Update filter coefficients
}

void DelayProcessor::setDistortion(float amount)
{
    distortionAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void DelayProcessor::setDistortionMix(float newMix)
{
    distortionMix = juce::jlimit(0.0f, 1.0f, newMix);
}

void DelayProcessor::setTempo(double bpm)
{
    tempo = juce::jlimit(20.0, 300.0, bpm);
}

void DelayProcessor::setHaasDelayMs(float ms)
{
    haasDelayMs = juce::jlimit(5.0f, 40.0f, ms);
}

// Helper methods
int DelayProcessor::msToSamples(float ms) const
{
    return static_cast<int>((ms / 1000.0f) * static_cast<float>(sampleRate));
}

float DelayProcessor::syncedDelayTime(SyncDivision division) const
{
    if (tempo <= 0.0)
        return 500.0f; // Default

    float beatsPerSecond = tempo / 60.0f;
    float secondsPerBeat = 1.0f / beatsPerSecond;
    float multiplier = 1.0f;

    switch (division)
    {
        case SyncDivision::Whole:          multiplier = 4.0f; break;
        case SyncDivision::Half:           multiplier = 2.0f; break;
        case SyncDivision::Quarter:        multiplier = 1.0f; break;
        case SyncDivision::Eighth:         multiplier = 0.5f; break;
        case SyncDivision::Sixteenth:      multiplier = 0.25f; break;
        case SyncDivision::DottedHalf:     multiplier = 3.0f; break;
        case SyncDivision::DottedQuarter:  multiplier = 1.5f; break;
        case SyncDivision::DottedEighth:   multiplier = 0.75f; break;
        case SyncDivision::TripletHalf:    multiplier = 4.0f / 3.0f; break;
        case SyncDivision::TripletQuarter: multiplier = 2.0f / 3.0f; break;
        case SyncDivision::TripletEighth:  multiplier = 1.0f / 3.0f; break;
    }

    return (secondsPerBeat * multiplier) * 1000.0f; // Convert to ms
}

void DelayProcessor::applyDistortion(float& sample)
{
    // Simple soft clipping distortion
    if (distortionAmount > 0.0f)
    {
        float threshold = 1.0f - distortionAmount * 0.7f;
        sample = std::tanh(sample / threshold) * threshold;
    }
}

void DelayProcessor::updateDistortionFunction()
{
    // Placeholder for waveshaper function update
}
