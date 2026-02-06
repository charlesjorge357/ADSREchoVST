/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"  // for Projucer
#else // for Cmake
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_audio_formats/juce_audio_formats.h>
  #include <juce_audio_plugin_client/juce_audio_plugin_client.h>
  #include <juce_audio_processors/juce_audio_processors.h>
  #include <juce_audio_utils/juce_audio_utils.h>
  #include <juce_core/juce_core.h>
  #include <juce_data_structures/juce_data_structures.h>
  #include <juce_dsp/juce_dsp.h>
  #include <juce_events/juce_events.h>
  #include <juce_graphics/juce_graphics.h>
  #include <juce_gui_basics/juce_gui_basics.h>
  #include <juce_gui_extra/juce_gui_extra.h>
#endif
#include "ModuleSlot.h"
#include "DelayModule.h"
#include "ReverbModule.h"
#include "IRBank.h"
#include "ConvolutionModule.h"

//==============================================================================
/**
*/


class ADSREchoAudioProcessor  : public juce::AudioProcessor, public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    ADSREchoAudioProcessor();
    ~ADSREchoAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    //std::vector<std::unique_ptr<ModuleSlot>> slots;
    std::vector<std::vector<std::unique_ptr<ModuleSlot>>> slots;

    int getNumSlots() const;
    int getNumChannels() const;
    SlotInfo getSlotInfo(int chainIndex, int slotIndex);
    bool slotIsEmpty(int chainIndex, int slotIndex);

    void addModule(int chainIndex, ModuleType moduleType);
    void removeModule(int chainIndex, int slotIndex);
    void changeModuleType(int chainIndex, int slotIndex, ModuleType moduleType);
    void requestSlotMove(int chainIndex, int from, int to);

    std::atomic<bool> uiNeedsRebuild{ false };

    // IR Bank accessor for UI
    std::shared_ptr<IRBank> getIRBank() const { return irBank; }

private:
    juce::dsp::ProcessSpec spec;

    std::shared_ptr<IRBank> irBank;

    // Pre-allocated buffer for dry signal (avoids allocation in processBlock)
    juce::AudioBuffer<float> masterDryBuffer;
    juce::AudioBuffer<float> chainTempBuffer;

    struct PendingMove
    {
        int chainIndex = -1;
        int from = -1;
        int to = -1;
    };

    std::atomic<bool> moveRequested{ false };
    PendingMove pendingMove;
    void executeSlotMove();

    static constexpr int MAX_SLOTS = 8;
    static constexpr int NUM_CHAINS = 2;

    void setSlotDefaults(juce::String slotID);

    std::vector<int> numModules = std::vector<int>(NUM_CHAINS, 0);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREchoAudioProcessor)
};
