#include "DatorroHall.h"

//==============================================================================
DatorroHall::DatorroHall() {}

DatorroHall::~DatorroHall() {}

//==============================================================================
void DatorroHall::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = (int) spec.sampleRate;

    // Prepare filters
    inputBandwidth.prepare(spec);
    feedbackDamping.prepare(spec);
    loopDamping.prepare(spec);

    inputBandwidth.reset();
    feedbackDamping.reset();
    loopDamping.reset();

    // Prepare all delay lines
    auto prepareDelay = [&](auto& d) { d.prepare(spec); d.reset(); };

    prepareDelay(inputZ);

    prepareDelay(loopDelayL1);
    prepareDelay(loopDelayL2);
    prepareDelay(loopDelayL3);
    prepareDelay(loopDelayL4);
    prepareDelay(loopDelayR1);
    prepareDelay(loopDelayR2);
    prepareDelay(loopDelayR3);
    prepareDelay(loopDelayR4);

    prepareDelay(allpassL1);
    prepareDelay(allpassL2);
    prepareDelay(allpassL3Inner);
    prepareDelay(allpassL3Outer);
    prepareDelay(allpassL4Innermost);
    prepareDelay(allpassL4Inner);
    prepareDelay(allpassL4Outer);

    prepareDelay(allpassR1);
    prepareDelay(allpassR2);
    prepareDelay(allpassR3Inner);
    prepareDelay(allpassR3Outer);
    prepareDelay(allpassR4Innermost);
    prepareDelay(allpassR4Inner);
    prepareDelay(allpassR4Outer);

    prepareDelay(allpassChorusL);
    prepareDelay(allpassChorusR);

    // Prepare LFO
    lfoParameters.frequency_Hz = 0.5f;
    lfoParameters.depth = 1.0f;
    lfo.setParameters(lfoParameters);
    lfo.reset(spec.sampleRate);

    // Resize channel vectors
    channelInput.assign(2, 0.0f);
    channelFeedback.assign(2, 0.0f);
    channelOutput.assign(2, 0.0f);
}

//==============================================================================
void DatorroHall::reset()
{
    inputBandwidth.reset();
    feedbackDamping.reset();
    loopDamping.reset();

    auto resetDelay = [&](auto& d) { d.reset(); };

    resetDelay(inputZ);

    resetDelay(loopDelayL1);
    resetDelay(loopDelayL2);
    resetDelay(loopDelayL3);
    resetDelay(loopDelayL4);
    resetDelay(loopDelayR1);
    resetDelay(loopDelayR2);
    resetDelay(loopDelayR3);
    resetDelay(loopDelayR4);

    resetDelay(allpassL1);
    resetDelay(allpassL2);
    resetDelay(allpassL3Inner);
    resetDelay(allpassL3Outer);
    resetDelay(allpassL4Innermost);
    resetDelay(allpassL4Inner);
    resetDelay(allpassL4Outer);

    resetDelay(allpassR1);
    resetDelay(allpassR2);
    resetDelay(allpassR3Inner);
    resetDelay(allpassR3Outer);
    resetDelay(allpassR4Innermost);
    resetDelay(allpassR4Inner);
    resetDelay(allpassR4Outer);

    resetDelay(allpassChorusL);
    resetDelay(allpassChorusR);

    lfo.reset(sampleRate);
}

//==============================================================================
void DatorroHall::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);

        for (int n = 0; n < numSamples; ++n)
        {
            const float input = data[n];

            // Input bandwidth filter
            const float inputFiltered = inputBandwidth.processSample(channel, input);

            // Early diffusion
            float ap1 = (channel == 0 ? allpassL1 : allpassR1).popSample(0);
            float feedback1 = ap1 * 0.75f * parameters.diffusion;
            (channel == 0 ? allpassL1 : allpassR1).pushSample(0, inputFiltered + feedback1);

            float ap2 = (channel == 0 ? allpassL2 : allpassR2).popSample(0);
            float feedback2 = ap2 * 0.406f * parameters.diffusion;
            (channel == 0 ? allpassL2 : allpassR2).pushSample(0, ap1 + feedback2);

            // LFO modulation
            const float lfoValue = static_cast<float>(lfo.renderAudioOutput().normalOutput);
            const float modDepth = parameters.modDepth;
            const float modulatedDelay = (1.0f + modDepth * lfoValue) * parameters.roomSize;

            // Main delay network (TODO: implement properly with custom delay class)
            float delayOutput = ap2;

            // Late diffusion & damping
            float damped = loopDamping.processSample(channel, delayOutput);
            float feedback = feedbackDamping.processSample(channel, damped);

            // Output & feedback
            channelOutput[channel] = feedback;
            data[n] = (input * (1.0f - parameters.mix)) + (feedback * parameters.mix);
        }
    }
}

//==============================================================================
ReverbProcessorParameters& DatorroHall::getParameters()
{
    return parameters;
}

//==============================================================================
void DatorroHall::setParameters(const ReverbProcessorParameters& params)
{
    parameters = params;

    parameters.roomSize = juce::jlimit(0.25f, 1.75f, parameters.roomSize);

    // Update filters
    inputBandwidth.setCutoffFrequency(parameters.inputBandwidth);
    feedbackDamping.setCutoffFrequency(parameters.damping);
    loopDamping.setCutoffFrequency(parameters.damping);

    // Update LFO
    lfoParameters.frequency_Hz = parameters.modRate;
    lfoParameters.depth = parameters.modDepth;
    lfo.setParameters(lfoParameters);
}
