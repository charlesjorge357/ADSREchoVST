/*
  ==============================================================================

    Compressor.h
    Created: 28 Feb 2026 12:18:58am
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


class Compressor : public juce::Component
{
    public:

        Compressor();
        ~Compressor() override = default;

        void paint(juce::Graphics&) override;
        void resized() override;

    private:
        juce::Label titleLabel;

        juce::Label thresholdLabel;
        juce::Label ratioLabel;
        juce::Label attackLabel;
        juce::Label releaseLabel;
        juce::Label inputLabel;
        juce::Label outputLabel;

        juce::Slider threshold;
        juce::Slider ratio;
        juce::Slider attack;
        juce::Slider release;
        juce::Slider input;
        juce::Slider output;


};