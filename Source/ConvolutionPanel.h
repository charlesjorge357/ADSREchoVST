/*
  ==============================================================================

    ConvolutionPanel.h
    Renamed from Convolve.h - visual panel for the Convolution module.
    Call attachToAPVTS() after construction to wire sliders and IR selector
    to APVTS.

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

#include "ConvolutionModule.h"
#include "PluginProcessor.h"
#include "Clookandfeel.h"

class ConvolutionPanel : public juce::Component
{
public:
    ConvolutionPanel();
    ~ConvolutionPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Call once after construction, from ModuleSlotEditor::buildEditor
    void attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& slotID,
                       ADSREchoAudioProcessor& processor,
                       int chainIndex,
                       int slotIndex);

private:
    CustomLNF lnf;
    // -------------------------------------------------------------------------
    // UI
    // -------------------------------------------------------------------------
    juce::Label    titleLabel;

    // IR selector - populated from IRBank in attachToAPVTS
    juce::ComboBox dropDown;
    juce::Label    dropDownLabel;
    juce::TextButton browseButton { "Browse" };

    // Shown when the saved IR index is missing from the user's IR bank
    juce::Label    irMissingLabel;

    juce::Slider   irGainSlider;
    juce::Slider   lowCutSlider;
    juce::Slider   highCutSlider;
    juce::Slider   preDelaySlider;
    juce::Slider   mixSlider;

    juce::Label    irGainLabel;
    juce::Label    lowCutLabel;
    juce::Label    highCutLabel;
    juce::Label    preDelayLabel;
    juce::Label    mixLabel;

    // -------------------------------------------------------------------------
    // APVTS attachments - created in attachToAPVTS
    // -------------------------------------------------------------------------
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        irGainAttach, lowCutAttach, highCutAttach, preDelayAttach, mixAttach;

    // IR selector is driven manually (no SliderAttachment - it uses a float
    // param but needs custom gesture handling and custom IR support)
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Stored so onChange lambda and browse callback can reach the module
    ADSREchoAudioProcessor* proc      = nullptr;
    int                     chainIdx  = -1;
    int                     slotIdx   = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionPanel)
};