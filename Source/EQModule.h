/*
  ==============================================================================

    EQModule.h
    Effect module for 3-band parametric EQ

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

#include "EffectModule.h"
#include "BasicEQ.h"

class EQModule : public EffectModule
{
public:
    EQModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts);

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    std::vector<juce::String> getUsedParameters() const override;

    juce::String getID() const override;
    void setID(juce::String& newID) override;
    juce::String getType() const override;

    // EQModule has no playhead dependency, but we keep the base default

private:
    juce::String moduleID;
    juce::AudioProcessorValueTreeState& state;
    BasicEQ eq;
};
