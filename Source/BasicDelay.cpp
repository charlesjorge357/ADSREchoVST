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

    for (int i = 0; i < numSamples; ++i)
    {
        // Left channel
        float inputL = leftChannel[i];
        float delayedL = delayLineL.popSample(0);

        delayLineL.pushSample(0, inputL + feedbackL * feedbackAmount);
        feedbackL = delayedL;

        float wetL = delayedL;
        leftChannel[i] = inputL * (1.0f - mixAmount) + wetL * mixAmount;

        // Right channel
        if (rightChannel != nullptr)
        {
            float inputR = rightChannel[i];
            float delayedR = delayLineR.popSample(0);

            delayLineR.pushSample(0, inputR + feedbackR * feedbackAmount);
            feedbackR = delayedR;

            float wetR = delayedR;
            rightChannel[i] = inputR * (1.0f - mixAmount) + wetR * mixAmount;
        }
    }
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
    delayTimeMs = delayMs;
    delayTimeSamples = (delayTimeMs / 1000.0f) * sampleRate;
    delayLineL.setDelay(delayTimeSamples);
    delayLineR.setDelay(delayTimeSamples);
}

void BasicDelay::setFeedback(float feedback)
{
    feedbackAmount = juce::jlimit(0.0f, 0.95f, feedback);
}

void BasicDelay::setMix(float mix)
{
    mixAmount = juce::jlimit(0.0f, 1.0f, mix);
}
