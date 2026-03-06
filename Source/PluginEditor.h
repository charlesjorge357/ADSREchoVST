/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

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

#include "PluginProcessor.h"
#include "DelayPanel.h"
#include "ConvolutionPanel.h"
#include "ReverbPanel.h"
//#include "Clookandfeel.h"
#include "equalizerPanel.h"
#include "Compressor.h"
#include "ModuleSlotEditor.h"
#include "EQModuleSlotEditor.h"
#include "CompressorModuleSlotEditor.h"

//==============================================================================
/**
*/


class ADSREchoAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer, public juce::AsyncUpdater
{
public:
    ADSREchoAudioProcessorEditor (ADSREchoAudioProcessor&);
    ~ADSREchoAudioProcessorEditor() override;

    //void changeListenerCallback(juce::ChangeBroadcaster*) override;
    //==============================================================================
    
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ADSREchoAudioProcessor& audioProcessor;

    //==============================================================================
       // Chain / module UI
    juce::Viewport moduleViewport;
    juce::Component moduleContainer; 
    
    /*CustomLNF customTypeFace;
    
    juce::Image customImage;


    ReverbPanel revebPanel;
    ConvolvePanel convolvePanel;
    DelayPanel delayPanel;
    EqualizerPanel equalizerPanel;
    Compressor compressorPanel;
    

    juce::TextButton reverbButton;
    juce::TextButton convolveButton;
    juce::TextButton delayButton;
    juce::TextButton equalizerButton;
    juce::TextButton compressorButton;
    */

    //juce::Slider masterMix;

    juce::TextButton addButton{ "+" };
    std::vector<std::unique_ptr<BaseModuleSlotEditor>> moduleEditors;

    juce::ComboBox chainSelector;
    int currentlyDisplayedChain = 0;

    //==============================================================================
    // Master controls (per chain)
    static constexpr int numChains = ADSREchoAudioProcessor::NUM_CHAINS;

    std::array<juce::Slider, numChains> masterMixSliders;
    std::array<juce::Slider, numChains> gainSliders;

    std::array<std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>, numChains>
        masterMixAttachments,
        gainAttachments;

    std::array<juce::Label, numChains> masterMixLabels, gainLabels;

    //==============================================================================
    // Parallel enable
    juce::ToggleButton parallelEnableToggle{ "Enabled" };
    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>
        parallelEnableToggleAttachment;

    //==============================================================================
    // Refactored helpers
    void rebuildModuleEditors();
    void setupChainControls(int chainIndex);

    //==============================================================================
    // Async + timer
    void timerCallback() override;
    void handleAsyncUpdate() override;

    bool attemptedChange = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSREchoAudioProcessorEditor)
};

