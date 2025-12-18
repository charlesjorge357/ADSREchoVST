#include "BasicDelay.h"

BasicDelay::BasicDelay() {}

BasicDelay::~BasicDelay() {}

void BasicDelay::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<float>(spec.sampleRate);

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    delayLineL.prepare(monoSpec);
    delayLineR.prepare(monoSpec);

    // Recalculate delay time in samples
    delayTimeSamples = (delayTimeMs / 1000.0f) * sampleRate;
    delayLineL.setDelay(delayTimeSamples);
    delayLineR.setDelay(delayTimeSamples);

    reset();
}

void BasicDelay::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Cache values to avoid repeated member access in tight loop
    const float wet = mixAmount;
    const float dry = 1.0f - mixAmount;
    const float fb = feedbackAmount;
    float fbL = feedbackL;
    float fbR = feedbackR;

    // Process left channel
    for (int i = 0; i < numSamples; ++i)
    {
        float inputL = leftChannel[i];
        float delayedL = delayLineL.popSample(0);
        delayLineL.pushSample(0, inputL + fbL * fb);
        fbL = delayedL;
        leftChannel[i] = inputL * dry + delayedL * wet;
    }

    // Process right channel separately (no branch in loop)
    if (rightChannel != nullptr)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float inputR = rightChannel[i];
            float delayedR = delayLineR.popSample(0);
            delayLineR.pushSample(0, inputR + fbR * fb);
            fbR = delayedR;
            rightChannel[i] = inputR * dry + delayedR * wet;
        }
    }

    // Store feedback state back
    feedbackL = fbL;
    feedbackR = fbR;
}

void BasicDelay::reset()
{
    delayLineL.reset();
    delayLineR.reset();
    feedbackL = 0.0f;
    feedbackR = 0.0f;
}

void BasicDelay::setDelayTime(float delayMs)
{
    if (delayTimeMs == delayMs)
        return;  // Skip if unchanged

    delayTimeMs = delayMs;
    delayTimeSamples = (delayTimeMs / 1000.0f) * sampleRate;
    delayLineL.setDelay(delayTimeSamples);
    delayLineR.setDelay(delayTimeSamples);
}

void BasicDelay::setFeedback(float feedback)
{
    float clamped = juce::jlimit(0.0f, 0.95f, feedback);
    if (feedbackAmount != clamped)
        feedbackAmount = clamped;
}

void BasicDelay::setMix(float mix)
{
    float clamped = juce::jlimit(0.0f, 1.0f, mix);
    if (mixAmount != clamped)
        mixAmount = clamped;
}
