/*
  ==============================================================================

    CompressorPanel.h
    Renamed from Compressor.h - visual panel for the Compressor module.
    Call attachToAPVTS() after construction (from CompressorModuleSlotEditor)
    to wire sliders to APVTS.

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

#include "Clookandfeel.h"

class CompressorPanel : public juce::Component
{
public:
    CompressorPanel();
    ~CompressorPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Call once from CompressorModuleSlotEditor::buildEditor, after the
    // display component has been set up, so the panel only owns the knobs
    void attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& slotID);

private:
    CustomLNF lnf;
    juce::Label titleLabel;

    juce::Slider threshold;
    juce::Slider ratio;
    juce::Slider attack;
    juce::Slider release;
    juce::Slider input;
    juce::Slider output;

    juce::Label thresholdLabel;
    juce::Label ratioLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;
    juce::Label inputLabel;
    juce::Label outputLabel;

    // APVTS attachments - created in attachToAPVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        thresholdAttach, ratioAttach, attackAttach,
        releaseAttach, inputAttach, outputAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorPanel)
};