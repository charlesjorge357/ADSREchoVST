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

void ReverbModule::setID(juce::String& newID) { moduleID = newID; rebuildParamIDs(); }

juce::String ReverbModule::getID() const { return moduleID; }
juce::String ReverbModule::getType() const { return "Reverb"; }