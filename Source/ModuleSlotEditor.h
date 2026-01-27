/*
  ==============================================================================
    ModuleSlotEditor.h - WITH IR COMBOBOX
  ==============================================================================
*/

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
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

    // Module Settings
    juce::Label title;
    juce::ComboBox typeSelector;
    juce::ToggleButton enableToggle{ "Enabled" };

    // Module Sliders
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> sliderLabels;
    
    // IR Selectors (ComboBoxes)
    std::vector<std::unique_ptr<juce::ComboBox>> irSelectors;
    std::vector<std::unique_ptr<juce::Label>> irSelectorLabels;
    
    juce::TextButton removeButton{ "-" };

    // Module Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableToggleAttachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    void addSliderForParameter(juce::String id);
    void addIRSelectorForParameter(juce::String id);
};