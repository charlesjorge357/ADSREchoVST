/*
  ==============================================================================

    CompressorModule.h
    Effect module for optical-style compressor

  ==============================================================================
*/

#pragma once
#include "EffectModule.h"
#include "BasicCompressor.h"

class CompressorModule : public EffectModule
{
public:
    CompressorModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts);

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    std::vector<juce::String> getUsedParameters() const override;

    juce::String getID()   const override;
    void setID(juce::String& newID) override;
    juce::String getType() const override;

    // -----------------------------------------------------------------------
    // Meter data - written by audio thread, read by UI thread (same pattern
    // as EQModule::fftReady / fftBuffer).
    // meterReady is set to true each process block so the editor can poll.
    // -----------------------------------------------------------------------
    std::atomic<float> inputLevelDb  { -100.0f };
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<bool>  meterReady    { false };

    // Expose threshold so the display can draw the threshold line
    float getThresholdDb() const;

private:
    juce::String moduleID;
    juce::AudioProcessorValueTreeState& state;
    BasicCompressor compressor;
};