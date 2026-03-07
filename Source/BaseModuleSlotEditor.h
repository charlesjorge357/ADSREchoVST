/*
  ==============================================================================
    ModuleSlotEditor.h
    Superclass for all module editors
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

class BaseModuleSlotEditor : public juce::Component
{
public:

    BaseModuleSlotEditor(
        int cIndex,
        int sIndex,
        const SlotInfo& info,
        ADSREchoAudioProcessor& processor,
        juce::AudioProcessorValueTreeState& apvts);

    virtual ~BaseModuleSlotEditor() = default;

    void paint(juce::Graphics&) override;
    virtual void resized() override;

protected:

    int chainIndex;
    int slotIndex;
    juce::String slotID;

    ADSREchoAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    // Shared UI

    juce::Label title;

    juce::ComboBox typeSelector;

    juce::ToggleButton enableToggle{ "Enabled" };

    juce::TextButton removeButton{ "-" };

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>
        enableToggleAttachment;

    // Subclass hooks

    virtual void buildEditor(const SlotInfo& info) = 0;

    virtual void layoutEditor(juce::Rectangle<int>& area) = 0;
};