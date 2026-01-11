/*
  ==============================================================================

    ModuleSlot.h
    Stores EffectModule along with params.

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

class ModuleSlot
{
public:
    explicit ModuleSlot(const juce::String& id)
        : slotID(id)
    {
    }
    
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        if (module)
            module->prepare(spec);
    }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        if (module && !bypassed)
            module->process(buffer, midi);
    }

    void setModule(std::unique_ptr<EffectModule> newModule,
        const juce::dsp::ProcessSpec& spec)
    {
        module = std::move(newModule);
        module->prepare(spec);
    }

    EffectModule* get() { return module.get(); }

    juce::String slotID;
    bool bypassed = false;

private:
    std::unique_ptr<EffectModule> module;
};