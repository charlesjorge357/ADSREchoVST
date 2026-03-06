/*
  ==============================================================================

    DelayPanel.h
    Created: 5 Jan 2026 12:47:08am
    Author:  ferna

  ==============================================================================
*/

#pragma once

#include "Clookandfeel.h"

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


class DelayPanel : public juce::Component
{
    public:
        DelayPanel();
        ~DelayPanel() override;

        void paint(juce::Graphics&) override;
        void resized() override;
        void updateToggleState(juce::Button*, juce::String);

        void attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& slotID);


    private:

        juce::Image delayBackground;

        CustomLNF lnk;

        juce::ComboBox noteDivsion;
        juce::ComboBox mode;
        juce::ToggleButton bmpTog {"BPM Sync"};
        //SvgLookAndFeel svgLookAndFeel;
        //juce::Typeface::Ptr myTypeface;

        
        juce::Label titleLabel;
        juce::Label feedbackLabel;
        juce::Label timeLabel;
        juce::Label bpmLabel;
        juce::Label panLabel;
        juce::Label lowpassLabel;
        juce::Label highpassLabel;
        juce::Label mixLabel;



        
        juce::Slider feedbackSlider;
        juce::Slider timeSlider;
        juce::Slider bpmSlider;
        juce::Slider panSlider;
        juce::Slider lowpassSlider;
        juce::Slider highpassSlider;
        juce::Slider mixSlider;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
            timeAttach, feedbackAttach, bpmAttach, panAttach,
            lowpassAttach, highpassAttach, mixAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
            noteDivAttach, modeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
            syncAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayPanel);




};
