/*
  ==============================================================================

    ConvolutionProcessor.cpp
    Convolution Reverb Module using impulse responses

  ==============================================================================
*/

#include "ConvolutionProcessor.h"

ConvolutionProcessor::ConvolutionProcessor()
{
}

ConvolutionProcessor::~ConvolutionProcessor()
{
}

void ConvolutionProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    convolutionL.prepare(monoSpec);
    convolutionR.prepare(monoSpec);

    // TODO: Load default impulse response
}

void ConvolutionProcessor::process(juce::AudioBuffer<float>& buffer)
{
    // TODO: Implement convolution processing
    juce::ignoreUnused(buffer);
}

void ConvolutionProcessor::reset()
{
    convolutionL.reset();
    convolutionR.reset();
}

void ConvolutionProcessor::loadImpulseResponse(const juce::File& irFile)
{
    // TODO: Implement IR loading from file
    juce::ignoreUnused(irFile);
}

void ConvolutionProcessor::loadImpulseResponse(const void* sourceData, size_t sourceDataSize)
{
    // TODO: Implement IR loading from memory
    juce::ignoreUnused(sourceData, sourceDataSize);
}

void ConvolutionProcessor::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);
}

void ConvolutionProcessor::setPreDelay(float preDelayMs)
{
    preDelayMs = juce::jlimit(0.0f, 500.0f, preDelayMs);
}
