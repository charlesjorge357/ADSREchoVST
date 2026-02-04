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

    // Prepare feedback path filters
    lowpassL.prepare(monoSpec);
    lowpassR.prepare(monoSpec);
    highpassL.prepare(monoSpec);
    highpassR.prepare(monoSpec);

    lowpassL.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    lowpassR.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    highpassL.setType(juce::dsp::FirstOrderTPTFilterType::highpass);
    highpassR.setType(juce::dsp::FirstOrderTPTFilterType::highpass);

    lowpassL.setCutoffFrequency(lowpassFreqValue);
    lowpassR.setCutoffFrequency(lowpassFreqValue);
    highpassL.setCutoffFrequency(highpassFreqValue);
    highpassR.setCutoffFrequency(highpassFreqValue);

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

    // Pre-compute per-block constants
    const float panGainL = 1.0f - juce::jmax(0.0f, panValue);
    const float panGainR = 1.0f + juce::jmin(0.0f, panValue);
    const float phaseSign = (delayMode == DelayMode::Inverted) ? -1.0f : 1.0f;
    const bool isPingPong = (delayMode == DelayMode::PingPong) && (rightChannel != nullptr);

    for (int i = 0; i < numSamples; ++i)
    {
        float inputL = leftChannel[i];
        float delayedL = delayLineL.popSample(0);

        if (rightChannel != nullptr)
        {
            float inputR = rightChannel[i];
            float delayedR = delayLineR.popSample(0);

            // Push with mode-dependent feedback routing
            // fbL/fbR already filtered from previous iteration
            if (isPingPong)
            {
                delayLineL.pushSample(0, inputL + fbR * fb);
                delayLineR.pushSample(0, inputR + fbL * fb);
            }
            else
            {
                delayLineL.pushSample(0, inputL + fbL * fb);
                delayLineR.pushSample(0, inputR + fbR * fb);
            }

            // Filter delayed signals for next iteration's feedback
            fbL = highpassL.processSample(0, lowpassL.processSample(0, delayedL));
            fbR = highpassR.processSample(0, lowpassR.processSample(0, delayedR));

            // Output with phase inversion and panning applied to wet signal
            leftChannel[i]  = inputL * dry + delayedL * phaseSign * wet * panGainL;
            rightChannel[i] = inputR * dry + delayedR * phaseSign * wet * panGainR;
        }
        else
        {
            // Mono path: no ping pong, no panning
            delayLineL.pushSample(0, inputL + fbL * fb);
            fbL = highpassL.processSample(0, lowpassL.processSample(0, delayedL));
            leftChannel[i] = inputL * dry + delayedL * phaseSign * wet;
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
    lowpassL.reset();
    lowpassR.reset();
    highpassL.reset();
    highpassR.reset();
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

void BasicDelay::setMode(DelayMode mode)
{
    delayMode = mode;
}

void BasicDelay::setPan(float pan)
{
    float clamped = juce::jlimit(-1.0f, 1.0f, pan);
    if (panValue != clamped)
        panValue = clamped;
}

void BasicDelay::setLowpassFreq(float freq)
{
    float clamped = juce::jlimit(200.0f, 20000.0f, freq);
    if (lowpassFreqValue != clamped)
    {
        lowpassFreqValue = clamped;
        lowpassL.setCutoffFrequency(lowpassFreqValue);
        lowpassR.setCutoffFrequency(lowpassFreqValue);
    }
}

void BasicDelay::setHighpassFreq(float freq)
{
    float clamped = juce::jlimit(20.0f, 5000.0f, freq);
    if (highpassFreqValue != clamped)
    {
        highpassFreqValue = clamped;
        highpassL.setCutoffFrequency(highpassFreqValue);
        highpassR.setCutoffFrequency(highpassFreqValue);
    }
}
