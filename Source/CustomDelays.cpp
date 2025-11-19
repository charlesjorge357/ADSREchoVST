#include "CustomDelays.h"
#include <algorithm>
#include <cmath>

//==============================================================================
// DelayLineWithSampleAccess
//==============================================================================

template <typename SampleType>
DelayLineWithSampleAccess<SampleType>::DelayLineWithSampleAccess(int maximumDelayInSamples)
{
    jassert(maximumDelayInSamples >= 0);

    totalSize = std::max(maximumDelayInSamples + 1, 4);
    delayBuffer.setSize(1, totalSize, false, false, false);
    delayBuffer.clear();

    numSamples = delayBuffer.getNumSamples();
}

template <typename SampleType>
DelayLineWithSampleAccess<SampleType>::~DelayLineWithSampleAccess() {}

template <typename SampleType>
void DelayLineWithSampleAccess<SampleType>::pushSample(int channel, SampleType newValue)
{
    delayBuffer.setSample(channel, writePosition[(size_t) channel], newValue);
    writePosition[(size_t) channel] = (writePosition[(size_t) channel] + 1) % numSamples;
}

template <typename SampleType>
SampleType DelayLineWithSampleAccess<SampleType>::popSample(int channel)
{
    readPosition[(size_t) channel] = juce::jlimit(
        0, numSamples - 1,
        (writePosition[(size_t) channel] - delayInSamples + numSamples) % numSamples
    );

    return delayBuffer.getSample(channel, readPosition[(size_t) channel]);
}

template <typename SampleType>
SampleType DelayLineWithSampleAccess<SampleType>::getSampleAtDelay(int channel, int delay) const
{
    return delayBuffer.getSample(channel,
        (writePosition[(size_t) channel] - delay + numSamples) % numSamples
    );
}

template <typename SampleType>
void DelayLineWithSampleAccess<SampleType>::setDelay(int newLength)
{
    if (newLength < 1)
        newLength = 1;

    delayInSamples = newLength;
    fractionalDelay = 0.0f;
}

template <typename SampleType>
void DelayLineWithSampleAccess<SampleType>::setDelay(float newDelayInSamples)
{
    if (newDelayInSamples < 1.0f)
        newDelayInSamples = 1.0f;

    if (newDelayInSamples > (float)(numSamples - 1))
        newDelayInSamples = (float)(numSamples - 1);

    delayInSamples = (int) std::floor(newDelayInSamples);
    fractionalDelay = newDelayInSamples - (float) delayInSamples;
}

template <typename SampleType>
SampleType DelayLineWithSampleAccess<SampleType>::readFractional(int channel, float delaySamples) const
{
    delaySamples = juce::jlimit(1.0f, (float)(numSamples - 1), delaySamples);

    const int writePos = writePosition[(size_t) channel];
    const int delayInt = (int) std::floor(delaySamples);
    const float frac = delaySamples - (float) delayInt;

    const int idx1 = (writePos - delayInt + numSamples) % numSamples;
    const int idx2 = (writePos - delayInt - 1 + numSamples) % numSamples;

    const SampleType s1 = delayBuffer.getSample(channel, idx1);
    const SampleType s2 = delayBuffer.getSample(channel, idx2);

    return s1 + frac * (s2 - s1);
}

template <typename SampleType>
void DelayLineWithSampleAccess<SampleType>::setSize(const int numChannels, const int newSize)
{
    totalSize = newSize;
    delayBuffer.setSize(numChannels, totalSize, false, false, true);
    reset();
}

template <typename SampleType>
int DelayLineWithSampleAccess<SampleType>::getNumSamples() const
{
    return delayBuffer.getNumSamples();
}

template <typename SampleType>
void DelayLineWithSampleAccess<SampleType>::prepare(const juce::dsp::ProcessSpec& spec)
{
    jassert(spec.numChannels > 0);

    delayBuffer.setSize((int) spec.numChannels, totalSize, false, false, true);

    writePosition.resize(spec.numChannels);
    readPosition.resize(spec.numChannels);
    v.resize(spec.numChannels);

    sampleRate = spec.sampleRate;

    reset();
}

template <typename SampleType>
void DelayLineWithSampleAccess<SampleType>::reset()
{
    std::fill(writePosition.begin(), writePosition.end(), 0);
    std::fill(readPosition.begin(), readPosition.end(), 0);
    std::fill(v.begin(), v.end(), (SampleType) 0);

    delayBuffer.clear();
}

//==============================================================================
// Allpass
//==============================================================================

template <typename SampleType>
Allpass<SampleType>::Allpass() = default;

template <typename SampleType>
Allpass<SampleType>::~Allpass() = default;

template <typename SampleType>
void Allpass<SampleType>::setMaximumDelayInSamples(int maxDelayInSamples)
{
    delayLine = DelayLineWithSampleAccess<SampleType>(maxDelayInSamples);
}

template <typename SampleType>
void Allpass<SampleType>::setDelay(SampleType newDelayInSamples)
{
    delayInSamples = (int) newDelayInSamples;
    delayLine.setDelay((float) newDelayInSamples);
}

template <typename SampleType>
void Allpass<SampleType>::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    delayLine.prepare(spec);

    drySample.resize(spec.numChannels);
    delayOutput.resize(spec.numChannels);
    feedforward.resize(spec.numChannels);
    feedback.resize(spec.numChannels);

    std::fill(drySample.begin(), drySample.end(), 0);
    std::fill(delayOutput.begin(), delayOutput.end(), 0);
    std::fill(feedforward.begin(), feedforward.end(), 0);
    std::fill(feedback.begin(), feedback.end(), 0);

    reset();
}

template <typename SampleType>
void Allpass<SampleType>::reset()
{
    delayLine.reset();
}

template <typename SampleType>
void Allpass<SampleType>::pushSample(int channel, SampleType sample)
{
    drySample[channel] = sample;
    delayLine.pushSample(channel, sample + feedback[channel]);
}

template <typename SampleType>
SampleType Allpass<SampleType>::popSample(int channel, SampleType overrideDelay, bool)
{
    const float delayToUse =
        (overrideDelay >= 0 ? overrideDelay : (float) delayInSamples);

    // fractional read
    delayOutput[channel] = delayLine.readFractional(channel, delayToUse);

    feedback[channel]    = delayOutput[channel] * gain;
    feedforward[channel] = -drySample[channel] - delayOutput[channel] * gain;

    return delayOutput[channel] + feedforward[channel];
}

template <typename SampleType>
void Allpass<SampleType>::setGain(SampleType newGain)
{
    gain = juce::jlimit<SampleType>(0.0, 1.0, newGain);
}

//==============================================================================
// Explicit template instantiation
//==============================================================================

template class DelayLineWithSampleAccess<float>;
template class DelayLineWithSampleAccess<double>;

template class Allpass<float>;
template class Allpass<double>;
