/*
  ==============================================================================

    ReverbPanel.h
    Created: 11 Jan 2026 9:16:53pm
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

#include "Clookandfeel.h"

class ReverbPanel : public juce::Component
{
    public:
        ReverbPanel();
        ~ReverbPanel() override;

        void paint(juce::Graphics&) override;
        void resized() override;

        void attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& slotID);


    private:
        CustomLNF lnf;
        juce::Label titleLabel;
        juce::ComboBox typeDrop;
        

        juce::Label roomSizeLabel;
        juce::Label decayLabel;
        juce::Label dampingLabel;
        juce::Label modRateLabel;
        juce::Label modDepthLabel;
        //juce::Label reverbDepthLabel;
        juce::Label preDelayLabel;
        juce::Label mixLabel;


        /*juce::Slider timeSlider;
        juce::Slider feedbackSlider;
        juce::Slider cutoffSlider;
        juce::Slider rateSlider;
        juce::Slider depthSlider;*/

        juce::Slider roomSize;
        juce::Slider decay;
        juce::Slider damping;
        juce::Slider modRate;
        juce::Slider modDepth;
        //juce::Slider reverbDepth;
        juce::Slider preDelay;
        juce::Slider Mix;

        juce::Image reverbBackground;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
            roomSizeAttach, decayAttach, dampingAttach, modRateAttach,
            modDepthAttach, preDelayAttach, mixAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
            typeAttach;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbPanel);


};
