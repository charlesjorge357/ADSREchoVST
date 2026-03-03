#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>
#endif
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
    juce::String getID()   const override;
    juce::String getType() const override;

    void setIRBank(std::shared_ptr<IRBank> bank);

    void loadCustomIR(const juce::File& file);
    void clearCustomIR();
    bool hasCustomIR()             const;
    juce::String getCustomIRPath() const;

private:
    juce::String moduleID;
    juce::AudioProcessorValueTreeState& state;

    Convolution convolutionReverb;

    // Pre-built parameter IDs - avoids String heap allocation every process block
    juce::String pMix, pPreDelay, pIrIndex, pIrGain, pLowCut, pHighCut, pEnabled;

    void rebuildParamIDs();
};