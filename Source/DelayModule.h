/*
  ==============================================================================

    DelayModule.h
    Effect module for delay

  ==============================================================================
*/

#pragma once
#include "EffectModule.h"
#include "BasicDelay.h"

class DelayModule : public EffectModule
{
public:
    DelayModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts);

    void prepare(const juce::dsp::ProcessSpec& spec) override;

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    std::vector<juce::String> getUsedParameters() const override;
    
    juce::String getID() const override;
    void setID(juce::String& newID) override;
    juce::String getType() const override;

    void setPlayHead(juce::AudioPlayHead *playhead) override;

    void rebuildParamCache();

private:
    juce::String moduleID;
    juce::AudioProcessorValueTreeState& state;
    juce::AudioPlayHead *playHead = nullptr;
    BasicDelay delay;

    // Cached parameter pointers — rebuilt in rebuildParamCache()
    std::atomic<float>* pMix             = nullptr;
    std::atomic<float>* pFeedback        = nullptr;
    std::atomic<float>* pSyncEnabled     = nullptr;
    std::atomic<float>* pBpm             = nullptr;
    std::atomic<float>* pNoteDiv         = nullptr;
    std::atomic<float>* pDelayTime       = nullptr;
    std::atomic<float>* pMode            = nullptr;
    std::atomic<float>* pPan             = nullptr;
    std::atomic<float>* pLowpass         = nullptr;
    std::atomic<float>* pHighpass        = nullptr;
    std::atomic<float>* pEnabled         = nullptr;
};