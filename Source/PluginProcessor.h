/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"  // for Projucer
#else
  #include <juce_audio_basics/juce_audio_basics.h> // for Cmake
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

// Processor modules
#include "DelayProcessor.h"
#include "ReverbProcessor.h"
#include "ConvolutionProcessor.h"
#include "CompressorProcessor.h"
#include "EQProcessor.h"
#include "RoutingMatrix.h"
#include "PresetManager.h"

//==============================================================================
/**
*/
class ADSREchoAudioProcessor  : public juce::AudioProcessor
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

    // Effect modules access
    auto& getDelayProcessor() { return delayProcessor; }
    auto& getReverbProcessor() { return reverbProcessor; }
    auto& getConvolutionProcessor() { return convolutionProcessor; }
    auto& getCompressorProcessor() { return compressorProcessor; }
    auto& getEQProcessor() { return eqProcessor; }

    // System access
    auto& getRoutingMatrix() { return routingMatrix; }
    auto& getPresetManager() { return presetManager; }

    // Tempo/BPM sync
    void updateTempo(double bpm);
    double getCurrentTempo() const { return currentTempo; }
    bool isTempoValid() const { return tempoValid; }

private:
    // Effect modules
    std::unique_ptr<DelayProcessor> delayProcessor;
    std::unique_ptr<ReverbProcessor> reverbProcessor;
    std::unique_ptr<ConvolutionProcessor> convolutionProcessor;
    std::unique_ptr<CompressorProcessor> compressorProcessor;
    std::unique_ptr<EQProcessor> eqProcessor;

    // System modules
    std::unique_ptr<RoutingMatrix> routingMatrix;
    std::unique_ptr<PresetManager> presetManager;

    // Tempo/BPM sync
    double currentTempo = 120.0;
    bool tempoValid = false;

    // Signal chain state
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> wetBuffer;

    // Helper methods
    void updateAllParameters();
    void routeSignalChain(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREchoAudioProcessor)
};
