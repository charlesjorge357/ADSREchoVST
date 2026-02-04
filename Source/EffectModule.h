/*
  ==============================================================================

    EffectModule.h
    Superclass for the module of each effect type.

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


class EffectModule
{
public:
    virtual ~EffectModule() = default;

    virtual void prepare(const juce::dsp::ProcessSpec&) = 0;
    virtual void process(juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0;

    virtual juce::String getType() const = 0;
    virtual juce::String getID() const = 0;
    virtual void setID(juce::String& newID) = 0;
    virtual void setPlayHead(juce::AudioPlayHead* playhead) {}

    virtual std::vector<juce::String> getUsedParameters() const = 0;
};