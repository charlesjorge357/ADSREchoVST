/*
  ==============================================================================

    ReverbModule.cpp
    Effect module for reverb

  ==============================================================================
*/

#include "ReverbModule.h"
ReverbModule::ReverbModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts) {
}

void ReverbModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    datorroReverb.prepare(spec);
    hybridPlateReverb.prepare(spec);
}

void ReverbModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    ReverbProcessorParameters params;
    params.mix = state.getRawParameterValue(moduleID + ".mix")->load();
    params.roomSize = state.getRawParameterValue(moduleID + ".roomSize")->load();
    params.decayTime = state.getRawParameterValue(moduleID + ".decayTime")->load();
    params.damping = state.getRawParameterValue(moduleID + ".damping")->load();
    params.modRate = state.getRawParameterValue(moduleID + ".modRate")->load();
    params.modDepth = state.getRawParameterValue(moduleID + ".modDepth")->load();
    params.preDelay = state.getRawParameterValue(moduleID + ".preDelay")->load();
    

    datorroReverb.setParameters(params);
    hybridPlateReverb.setParameters(params);

    if (*state.getRawParameterValue(moduleID + ".enabled") == true) 
    { 
        if (static_cast<int>(state.getRawParameterValue(moduleID + ".reverbType")->load()) == 0)
        {
            datorroReverb.processBlock(buffer, midi);
        }
        else 
        {
            hybridPlateReverb.processBlock(buffer, midi);
        }
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

void ReverbModule::setID(juce::String& newID) { moduleID = newID; }

juce::String ReverbModule::getID() const { return moduleID; }
juce::String ReverbModule::getType() const { return "Reverb"; }