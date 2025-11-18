// HybridPlate.cpp
#include "HybridPlate.h"
#include <algorithm>
#include "CustomDelays.h"
#include "LFO.h"
#include "ProcessorBase.h"
#include "Utilities.h"

HybridPlate::HybridPlate() = default;
HybridPlate::~HybridPlate() = default;

void HybridPlate::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Resize & prepare diffusers (short delays used as allpass-like diffusers)
    diffusers.clear();
    diffusers.resize(diffuserCount);
    for (size_t i = 0; i < diffuserCount; ++i)
    {
        diffusers[i].prepare(spec);
        diffusers[i].setMaximumDelayInSamples(static_cast<int>(sampleRate * 0.1));
        diffusers[i].reset();
    }

    // Prepare FDN delay lines: use DelayLineWithSampleAccess API
    for (int i = 0; i < fdnCount; ++i)
    {
        fdnLines[i].prepare(spec);
        // Note: DelayLineWithSampleAccess doesn't have setMaximumDelayInSamples
        // The size is set in constructor (44100) which is already sufficient
        fdnLines[i].reset();
    }

    // Prepare damping filters
    for (auto& f : dampingFilters)
    {
        f.prepare(spec);
        f.reset();
        f.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    }

    // LFO
    lfoParameters.frequency_Hz = 0.2;
    lfoParameters.waveform = generatorWaveform::sin;
    lfo.setParameters(lfoParameters);
    lfo.prepare(spec);
    lfo.reset(sampleRate);

    // Reset internal states
    reset();
}

void HybridPlate::reset()
{
    for (auto& d : diffusers)
        d.reset();

    for (auto& fdn : fdnLines)
        fdn.reset();

    for (auto& f : dampingFilters)
        f.reset();

    lfo.reset(sampleRate);
}

ReverbProcessorParameters& HybridPlate::getParameters() { return parameters; }

void HybridPlate::setParameters(const ReverbProcessorParameters& params)
{
    if (!(params == parameters))
    {
        parameters = params;

        // optional mapping / scaling: match what Freeverb did for roomSize mapping if desired
        parameters.roomSize = scale(parameters.roomSize, 0.0f, 1.0f, 0.25f, 1.75f);

        // update LFO parameters locally
        lfoParameters.frequency_Hz = parameters.modRate;
        lfo.setParameters(lfoParameters);
    }
}

void HybridPlate::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // update damping filters cutoff from parameters each block
    for (auto& f : dampingFilters)
        f.setCutoffFrequency(parameters.damping);

    // update LFO
    lfoParameters = lfo.getParameters();
    lfoParameters.frequency_Hz = parameters.modRate;
    lfo.setParameters(lfoParameters);

    // per-channel processing
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);

        // compute a small channel offset for stereo spread
        float channelOffset = static_cast<float>(ch) * stereoWidth;

        // sample loop
        for (int n = 0; n < numSamples; ++n)
        {
            float x = channelData[n];

            // Get LFO modulation (once per sample)
            SignalGenData lfoOut = lfo.renderAudioOutput();
            float mod = static_cast<float>(lfoOut.normalOutput) * parameters.modDepth;

            // Set diffuser delays with modulation
            for (size_t i = 0; i < diffuserCount; ++i)
            {
                float delaySamples = diffuserDelayTimes[i] + (mod * 2.0f); // small mod
                diffusers[i].setDelay(std::max(1.0f, delaySamples));
            }

            // Set FDN delays (scaled by roomSize and add small stereo offset)
            for (size_t i = 0; i < fdnCount; ++i)
            {
                float dt = fdnDelayTimes[i] * parameters.roomSize + channelOffset;
                fdnLines[i].setDelay(static_cast<int>(std::max(16.0f, dt)));
            }

            // --- serial diffusers (allpass-like)
            float diffused = x;
            for (size_t i = 0; i < diffuserCount; ++i)
            {
                // pop current delayed output - use channel 0 for mono delay lines
                float delayed = diffusers[i].popSample(0);
                // create a simple allpass-like feedback
                float feedback = delayed * -0.7f;
                float vn = diffused + feedback;
                diffusers[i].pushSample(0, vn);
                diffused = delayed + (vn * 0.7f);
            }

            // --- FDN section
            // read current outputs from fdn lines
            float fdnOut[fdnCount];
            for (int i = 0; i < fdnCount; ++i)
                fdnOut[i] = fdnLines[i].popSample(0);

            // compute feedback vector via feedbackMatrix
            float fb[fdnCount] = {0.0f, 0.0f, 0.0f, 0.0f};
            for (int i = 0; i < fdnCount; ++i)
            {
                float sum = 0.0f;
                for (int j = 0; j < fdnCount; ++j)
                    sum += feedbackMatrix[i][j] * fdnOut[j];
                fb[i] = sum * parameters.decayTime; // scale by decay
            }

            // push new samples into fdn with damping
            for (int i = 0; i < fdnCount; ++i)
            {
                float newSample = diffused + fb[i];
                // process damping filter (per-line) - use channel 0 for mono filters
                float damped = dampingFilters[i].processSample(0, newSample);
                fdnLines[i].pushSample(0, damped);
            }

            // mix FDN outputs to form tail (average)
            float tail = 0.0f;
            for (int i = 0; i < fdnCount; ++i)
                tail += fdnOut[i];
            tail *= (1.0f / static_cast<float>(fdnCount));

            // final wet/dry mix - use 'mix' parameter like DatorroHall
            channelData[n] = (x * (1.0f - parameters.mix)) + (tail * parameters.mix);
        } // samples
    } // channels
}