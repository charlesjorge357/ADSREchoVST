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
#include "ModuleSlotEditor.h"

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

    juce::Viewport moduleViewport;
    juce::Component moduleContainer;

    juce::TextButton addButton{ "+" };
    juce::OwnedArray<ModuleSlotEditor> moduleEditors;

    //Master Sliders
    juce::Slider masterMixSlider, gainSlider;

    //Master Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    //Master Labels
    juce::Label masterMixLabel, gainLabel;

    void rebuildModuleEditors();
    void timerCallback() override;
    void handleAsyncUpdate() override;

    bool attemptedChange = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREchoAudioProcessorEditor)
};
