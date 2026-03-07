/*
  ==============================================================================

    MasterPanel.h
    Per-chain master mix and gain controls.
    Call attachToAPVTS() after construction to wire sliders to APVTS.

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

class MasterPanel : public juce::Component
{
public:
    MasterPanel(int chainIndex);
    ~MasterPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void attachToAPVTS(juce::AudioProcessorValueTreeState& apvts);

private:
    CustomLNF lnf;
    int chainIndex;

    juce::Label titleLabel;

    juce::Slider mixSlider;
    juce::Slider gainSlider;

    juce::Label mixLabel;
    juce::Label gainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        mixAttach, gainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterPanel)
};
