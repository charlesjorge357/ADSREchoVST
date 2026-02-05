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
#include "Convolution.h"
#include "ModuleSlot.h"
#include "DelayModule.h"
#include "DatorroModule.h"
#include "IRBank.h"
#include "HybridPlateModule.h"
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

    void routeSignalChain(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    std::vector<std::unique_ptr<ModuleSlot>> slots;

    int getNumSlots() const;
    int getNumChannels() const;
    SlotInfo getSlotInfo(int index);
    bool slotIsEmpty(int index);

    void addModule(ModuleType moduleType);
    void removeModule(int slotIndex);
    void changeModuleType(int slotIndex, int newType);

    std::atomic<bool> uiNeedsRebuild{ false };

    // IR Bank accessor for UI
    std::shared_ptr<IRBank> getIRBank() const { return irBank; }

private:
    juce::dsp::ProcessSpec spec;
    
    DatorroHall datorroReverb;
    HybridPlate hybridReverb;
    BasicDelay basicDelay;
    Convolution convolutionReverb;
    std::unique_ptr<RoutingMatrix> routingMatrix;

    std::shared_ptr<IRBank> irBank;

    // Pre-allocated buffer for dry signal (avoids allocation in processBlock)
    juce::AudioBuffer<float> masterDryBuffer;

    struct PendingMove
    {
        int from = -1;
        int to = -1;
    };

    std::atomic<bool> moveRequested{ false };
    PendingMove pendingMove;
    void requestSlotMove(int from, int to);

    static constexpr int MAX_SLOTS = 8;
    static constexpr int CHANNELS = 2;

    void setSlotDefaults(juce::String slotID);

    int numModules = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREchoAudioProcessor)
};
