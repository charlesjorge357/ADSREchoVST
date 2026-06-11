/*
  ==============================================================================

    ReverbModule.cpp
    Effect module for reverb

  ==============================================================================
*/

#include "ReverbModule.h"
ReverbModule::ReverbModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts)
{
    rebuildParamIDs();
}

void ReverbModule::rebuildParamIDs()
{
    pMix        = state.getRawParameterValue(moduleID + ".mix");
    pRoomSize   = state.getRawParameterValue(moduleID + ".roomSize");
    pDecayTime  = state.getRawParameterValue(moduleID + ".decayTime");
    pDamping    = state.getRawParameterValue(moduleID + ".damping");
    pModRate    = state.getRawParameterValue(moduleID + ".modRate");
    pModDepth   = state.getRawParameterValue(moduleID + ".modDepth");
    pPreDelay   = state.getRawParameterValue(moduleID + ".preDelay");
    pEnabled    = state.getRawParameterValue(moduleID + ".enabled");
    pReverbType = state.getRawParameterValue(moduleID + ".reverbType");
}

void ReverbModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    datorroReverb.prepare(spec);
    hybridPlateReverb.prepare(spec);
}

void ReverbModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (pEnabled->load() < 0.5f)
        return;

    ReverbProcessorParameters params;
    params.mix       = pMix->load();
    params.roomSize  = pRoomSize->load();
    params.decayTime = pDecayTime->load();
    params.damping   = pDamping->load();
    params.modRate   = pModRate->load();
    params.modDepth  = pModDepth->load();
    params.preDelay  = pPreDelay->load();

    if (static_cast<int>(pReverbType->load()) == 0)
    {
        datorroReverb.setParameters(params);
        datorroReverb.processBlock(buffer, midi);
    }
    else
    {
        hybridPlateReverb.setParameters(params);
        hybridPlateReverb.processBlock(buffer, midi);
    }
}

std::vector<juce::String> ReverbModule::getUsedParameters() const
{
    return {
       "mix",
       "reverbType",
       "roomSize",
       "decayTime",
       "damping",
       "modRate",
       "modDepth",
       "preDelay"
    };
}

void ReverbModule::setID(const juce::String& newID) { moduleID = newID; rebuildParamIDs(); }

int ReverbModule::getTailLengthSamples(double sampleRate) const
{
    // decayTime is T60 (-60 dB). The silence gate skips only below ~-120 dB,
    // so flush 2x T60, with a 25% margin for the modal deviations of the
    // FDN/allpass networks (roomSize stretches loop delays). Plus pre-delay.
    const float t60   = pDecayTime ? pDecayTime->load() : 10.0f;
    const float preMs = pPreDelay  ? pPreDelay->load()  : 200.0f;
    return (int)(sampleRate * (2.5 * (double) t60 + (double) preMs * 0.001));
}

juce::String ReverbModule::getID() const { return moduleID; }
juce::String ReverbModule::getType() const { return "Reverb"; }