/*
  ==============================================================================

    EQPanel.h
    Renamed from equalizerPanel.h - visual panel for the EQ module.
    Call attachToAPVTS() after construction (from EQModuleSlotEditor)
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

class EQPanel : public juce::Component
{
public:
    EQPanel();
    ~EQPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Call once from EQModuleSlotEditor::buildEditor
    void attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& slotID);

private:
    CustomLNF lnf;
    juce::Label equalizerLabel;

    juce::Slider lowFreq;
    juce::Slider lowGain;
    juce::Slider lowQ;
    juce::Slider midFreq;
    juce::Slider midGain;
    juce::Slider midQ;
    juce::Slider highFreq;
    juce::Slider highGain;
    juce::Slider highQ;

    juce::Label lowFreqLabel;
    juce::Label lowGainLabel;
    juce::Label lowQLabel;
    juce::Label midFreqLabel;
    juce::Label midGainLabel;
    juce::Label midQLabel;
    juce::Label highFreqLabel;
    juce::Label highGainLabel;
    juce::Label highQLabel;

    // APVTS attachments - created in attachToAPVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        lowFreqAttach,  lowGainAttach,  lowQAttach,
        midFreqAttach,  midGainAttach,  midQAttach,
        highFreqAttach, highGainAttach, highQAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQPanel)
};