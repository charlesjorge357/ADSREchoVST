/*
  ==============================================================================

    DattoroModule.h
    Effect module for hybrid plate

  ==============================================================================
*/

#pragma once
#include "EffectModule.h"
#include "HybridPlate.h"

class HybridPlateModule : public EffectModule
{
public:
    HybridPlateModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts);

    void prepare(const juce::dsp::ProcessSpec& spec) override;

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    std::vector<juce::String> getUsedParameters() const override;

    juce::String getID() const override;
    void setID(juce::String& newID) override;
    juce::String getType() const override;

private:
    juce::String moduleID;
    juce::AudioProcessorValueTreeState& state;
    HybridPlate hybridPlateReverb;
};