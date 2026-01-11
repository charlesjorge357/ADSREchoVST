/*
  ==============================================================================

    ModuleSlotEditor.h
    UI Component Class for Effect Module Slots

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

class ModuleSlotEditor : public juce::Component
{
public:
    ModuleSlotEditor(int index,
        const SlotInfo& info,
        ADSREchoAudioProcessor& processor,
        juce::AudioProcessorValueTreeState& apvts);

    void resized() override;

private:
    int slotIndex;
    juce::String slotID;

    ADSREchoAudioProcessor& processor;

    juce::Label title;
    juce::ToggleButton enableToggle{ "Enabled" };

    juce::Slider mixSlider;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    juce::TextButton removeButton{ "-" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableToggleAttachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    void addSliderForParameter(juce::String id);
};