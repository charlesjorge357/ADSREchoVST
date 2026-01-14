/*
  ==============================================================================

    DattoroModule.cpp
    Effect module for dattoro hall

  ==============================================================================
*/

#include "DatorroModule.h"
DatorroModule::DatorroModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts) {
}

void DatorroModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    datorroReverb.prepare(spec);
}

void DatorroModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
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

    if (*state.getRawParameterValue(moduleID + ".enabled") == true) { datorroReverb.processBlock(buffer, midi); }

}

std::vector<juce::String> DatorroModule::getUsedParameters() const
{
    return {
       "mix",
       "room size",
       "decay time",
       "damping",
       "mod rate",
       "mod depth",
       "pre delay"
    };
}

void DatorroModule::setID(juce::String& newID) { moduleID = newID; }

juce::String DatorroModule::getID() const { return moduleID; }
juce::String DatorroModule::getType() const { return "Datorro Hall"; }