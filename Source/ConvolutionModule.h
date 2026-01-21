#pragma once

#include <JuceHeader.h>
#include "EffectModule.h"
#include "Convolution.h"
#include "IRBank.h"

class ConvolutionModule : public EffectModule
{
public:
    ConvolutionModule(const juce::String& id,
                      juce::AudioProcessorValueTreeState& apvts);

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void process(juce::AudioBuffer<float>& buffer,
                 juce::MidiBuffer& midi) override;

    std::vector<juce::String> getUsedParameters() const override;

    void setID(juce::String& newID) override;
    juce::String getID() const override;
    juce::String getType() const override;
    
    void setIRBank(std::shared_ptr<IRBank> bank);

private:
    juce::String moduleID;
    juce::AudioProcessorValueTreeState& state;

    Convolution convolutionReverb;
};