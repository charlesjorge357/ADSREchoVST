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
    params.roomSize = state.getRawParameterValue(moduleID + ".room size")->load();
    params.decayTime = state.getRawParameterValue(moduleID + ".decay time")->load();
    params.damping = state.getRawParameterValue(moduleID + ".damping")->load();
    params.modRate = state.getRawParameterValue(moduleID + ".mod rate")->load();
    params.modDepth = state.getRawParameterValue(moduleID + ".mod depth")->load();
    params.preDelay = state.getRawParameterValue(moduleID + ".pre delay")->load();
    

    datorroReverb.setParameters(params);
    hybridPlateReverb.setParameters(params);

    if (*state.getRawParameterValue(moduleID + ".enabled") == true) 
    { 
        if (static_cast<int>(state.getRawParameterValue(moduleID + ".reverb type")->load()) == 0)
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
       "reverb type",
       "room size",
       "decay time",
       "damping",
       "mod rate",
       "mod depth",
       "pre delay"
    };
}

void ReverbModule::setID(juce::String& newID) { moduleID = newID; }

juce::String ReverbModule::getID() const { return moduleID; }
juce::String ReverbModule::getType() const { return "Reverb"; }