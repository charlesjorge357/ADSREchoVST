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
        currentSpec = spec;

        if (auto* m = activeModule.load(std::memory_order_acquire))
            m->prepare(spec);
    }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        if (auto* m = activeModule.load(std::memory_order_acquire))
            m->process(buffer, midi);
    }

    void setModule(std::unique_ptr<EffectModule> newModule)
    {
        if (newModule)
        {
            newModule->prepare(currentSpec);
            newModule->setID(slotID);
        }

        // Keep old module alive until after swap
        pendingDeletion = std::move(ownedModule);
        ownedModule = std::move(newModule);

        // Atomic pointer swap (audio thread safe)
        activeModule.store(ownedModule.get(), std::memory_order_release);
    }

    void clearModule()
    {
        pendingDeletion = std::move(ownedModule);
        activeModule.store(nullptr, std::memory_order_release);
    }

    void destroyPending()
    {
        pendingDeletion.reset();
    }

    EffectModule* get() { return ownedModule.get(); }

    juce::String slotID;
    bool bypassed = false;

private:
    juce::dsp::ProcessSpec currentSpec{};

    std::unique_ptr<EffectModule> ownedModule;
    std::unique_ptr<EffectModule> pendingDeletion;

    std::atomic<EffectModule*> activeModule{ nullptr };
};