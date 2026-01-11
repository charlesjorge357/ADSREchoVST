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
#include "DatorroHall.h"
#include "HybridPlate.h"
#include "RoutingMatrix.h"
#include "BasicDelay.h"
#include "ModuleSlot.h"
#include "DelayModule.h"

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

    void routeSignalChain(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    std::vector<std::unique_ptr<ModuleSlot>> slots;

    int getNumSlots() const;
    SlotInfo getSlotInfo(int index);
    bool slotIsEmpty(int index);

    void requestAddModule(ModuleType type);
    void requestRemoveModule(int slotIndex);

private:
    juce::dsp::ProcessSpec spec;
    
    DatorroHall datorroReverb;
    HybridPlate hybridReverb;
    BasicDelay basicDelay;
    std::unique_ptr<RoutingMatrix> routingMatrix;

    // Pre-allocated buffer for dry signal (avoids allocation in processBlock)
    juce::AudioBuffer<float> masterDryBuffer;

    struct PendingChange
    {
        enum Type { Add, Remove } type;
        ModuleType moduleType;
        int slotIndex = -1;
    };

    std::atomic<bool> hasPendingChange{ false };
    PendingChange pendingChange;
    std::unique_ptr<EffectModule> pendingModule;

    static constexpr int MAX_SLOTS = 8;

    void applyPendingChange();
    void addModule(ModuleType moduleType);

    int numModules = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREchoAudioProcessor)
};
