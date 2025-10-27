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

//==============================================================================
/**
*/
class ADSREchoAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ADSREchoAudioProcessorEditor (ADSREchoAudioProcessor&);
    ~ADSREchoAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ADSREchoAudioProcessor& audioProcessor;

    // Global Sliders
    juce::Slider outputGainSlider;
    juce::Slider dryWetMixSlider;

    // Delay Sliders
    juce::Slider delayTimeLeftSlider;
    juce::Slider delayTimeRightSlider;
    juce::Slider delayFeedbackSlider;
    juce::Slider delayMixSlider;

    // Reverb Sliders
    juce::Slider reverbSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider reverbWidthSlider;
    juce::Slider reverbMixSlider;
    juce::Slider reverbPreDelaySlider;

    // Compressor Sliders
    juce::Slider compressorThresholdSlider;
    juce::Slider compressorRatioSlider;
    juce::Slider compressorAttackSlider;
    juce::Slider compressorReleaseSlider;

    // EQ Sliders
    juce::Slider eqLowFreqSlider;
    juce::Slider eqLowGainSlider;
    juce::Slider eqMidFreqSlider;
    juce::Slider eqMidGainSlider;
    juce::Slider eqHighFreqSlider;
    juce::Slider eqHighGainSlider;

    // Convolution Slider
    juce::Slider convolutionMixSlider;

    // Labels
    juce::Label outputGainLabel, dryWetMixLabel;
    juce::Label delayTimeLeftLabel, delayTimeRightLabel, delayFeedbackLabel, delayMixLabel;
    juce::Label reverbSizeLabel, reverbDampingLabel, reverbWidthLabel, reverbMixLabel, reverbPreDelayLabel;
    juce::Label compressorThresholdLabel, compressorRatioLabel, compressorAttackLabel, compressorReleaseLabel;
    juce::Label eqLowFreqLabel, eqLowGainLabel, eqMidFreqLabel, eqMidGainLabel, eqHighFreqLabel, eqHighGainLabel;
    juce::Label convolutionMixLabel;

    // Section Labels
    juce::Label globalLabel, delayLabel, reverbLabel, compressorLabel, eqLabel, convolutionLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeLeftAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeRightAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbPreDelayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compressorThresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compressorRatioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compressorAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compressorReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqLowFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqLowGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqMidFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqMidGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqHighFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> eqHighGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> convolutionMixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREchoAudioProcessorEditor)
};
