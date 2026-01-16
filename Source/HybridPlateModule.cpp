/*
  ==============================================================================

    DattoroModule.cpp
    Effect module for hybrid plate

  ==============================================================================
*/

#include "HybridPlateModule.h"
HybridPlateModule::HybridPlateModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts) {
}

void HybridPlateModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    hybridPlateReverb.prepare(spec);
}

void HybridPlateModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    ReverbProcessorParameters params;
    params.mix = state.getRawParameterValue(moduleID + ".mix")->load();
    params.roomSize = state.getRawParameterValue(moduleID + ".room size")->load();
    params.decayTime = state.getRawParameterValue(moduleID + ".decay time")->load();
    params.damping = state.getRawParameterValue(moduleID + ".damping")->load();
    params.modRate = state.getRawParameterValue(moduleID + ".mod rate")->load();
    params.modDepth = state.getRawParameterValue(moduleID + ".mod depth")->load();
    params.preDelay = state.getRawParameterValue(moduleID + ".pre delay")->load();


    hybridPlateReverb.setParameters(params);

    if (*state.getRawParameterValue(moduleID + ".enabled") == true) { hybridPlateReverb.processBlock(buffer, midi); }

}

std::vector<juce::String> HybridPlateModule::getUsedParameters() const
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

void HybridPlateModule::setID(juce::String& newID) { moduleID = newID; }

juce::String HybridPlateModule::getID() const { return moduleID; }
juce::String HybridPlateModule::getType() const { return "Hybrid Plate"; }