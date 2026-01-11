/*
  ==============================================================================

    DelayModule.cpp
    Effect module for delay

  ==============================================================================
*/

#include "DelayModule.h"
DelayModule::DelayModule(const juce::String& id, juce::AudioProcessorValueTreeState & apvts)
    : moduleID(id), state(apvts) {
}

void DelayModule::prepare(const juce::dsp::ProcessSpec & spec)
{
    delay.prepare(spec);
}

void DelayModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    delay.setMix( *state.getRawParameterValue(moduleID + ".mix"));
    delay.setDelayTime( *state.getRawParameterValue(moduleID + ".delay time"));
    delay.setFeedback( *state.getRawParameterValue(moduleID + ".feedback"));

    delay.processBlock(buffer);
}

std::vector<juce::String> DelayModule::getUsedParameters() const
{
    return {
       "mix",
       "delay time",
       "feedback",
    };
}

juce::String DelayModule::getID() const { return moduleID; }
juce::String DelayModule::getType() const { return "Delay"; }