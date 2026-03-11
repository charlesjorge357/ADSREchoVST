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
    void setCustomIRPathDeferred(const juce::File& file);
    void clearCustomIR();
    bool hasCustomIR()             const;
    juce::String getCustomIRPath() const;

    // Forces the convolver to reload the IR at the given bank index,
    // bypassing the cached-index guard. Used after preset restore.
    void forceReloadIR(int index);

    // True when the last requested bank IR was not found (out of range or file missing).
    bool isIRMissing() const;

private:
    juce::String moduleID;
    juce::AudioProcessorValueTreeState& state;

    Convolution convolutionReverb;

    // Cached raw pointers — built once in rebuildParamIDs(), avoids HashMap lookup every block
    std::atomic<float>* pMix      = nullptr;
    std::atomic<float>* pPreDelay = nullptr;
    std::atomic<float>* pIrIndex  = nullptr;
    std::atomic<float>* pIrGain   = nullptr;
    std::atomic<float>* pLowCut   = nullptr;
    std::atomic<float>* pHighCut  = nullptr;
    std::atomic<float>* pEnabled  = nullptr;

    void rebuildParamIDs();
};