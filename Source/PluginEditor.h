/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"  // for Projucer
#else
  #include <juce_audio_processors/juce_audio_processors.h> // for CMake
  #include <juce_dsp/juce_dsp.h>
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREchoAudioProcessorEditor)
};
