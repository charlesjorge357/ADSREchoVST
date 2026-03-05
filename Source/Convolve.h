/*
  ==============================================================================

    Convolve.h
    Created: 11 Jan 2026 7:33:27pm
    Author:  ferna

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


class ConvolvePanel : public juce::Component
{
    public:
        ConvolvePanel();
        ~ConvolvePanel() override = default;

        void paint(juce::Graphics&) override;
        void resized() override;


    private:
        juce::Label titleLabel;

        juce::ComboBox dropDown; 
        
        juce::Slider preDelaySlider;
        juce::Slider irGainSlider;
        juce::Slider lowCutSlider;
        juce::Slider highCutSlider;
        juce::Slider mixSlider;


        juce::Label preDelay;
        juce::Label irGain;
        juce::Label lowCut;
        juce::Label highCut;
        juce::Label mix;

        //juce::Image convoBackground;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolvePanel);




};
