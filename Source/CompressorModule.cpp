/*
  ==============================================================================

    CompressorModule.cpp
    Effect module for optical-style compressor

  ==============================================================================
*/

#include "CompressorModule.h"

CompressorModule::CompressorModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts)
{
}

void CompressorModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    compressor.prepare(spec);
}

void CompressorModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    compressor.setThreshold  (state.getRawParameterValue(moduleID + ".compThreshold")->load());
    compressor.setRatio      (state.getRawParameterValue(moduleID + ".compRatio")    ->load());
    compressor.setAttack     (state.getRawParameterValue(moduleID + ".compAttack")   ->load());
    compressor.setRelease    (state.getRawParameterValue(moduleID + ".compRelease")  ->load());
    compressor.setInputGain  (state.getRawParameterValue(moduleID + ".compInput")    ->load());
    compressor.setOutputGain (state.getRawParameterValue(moduleID + ".compOutput")   ->load());

    if (*state.getRawParameterValue(moduleID + ".enabled") > 0.5f)
        compressor.processBlock(buffer);
}

std::vector<juce::String> CompressorModule::getUsedParameters() const
{
    return {
        "compThreshold",
        "compRatio",
        "compAttack",
        "compRelease",
        "compInput",
        "compOutput"
    };
}

void CompressorModule::setID(juce::String& newID) { moduleID = newID; }

juce::String CompressorModule::getID()   const { return moduleID; }
juce::String CompressorModule::getType() const { return "Compressor"; }