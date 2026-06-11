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
#include "ConvolutionModule.h"

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

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, juce::AudioPlayHead* playHead)
    {
        if (auto* m = activeModule.load(std::memory_order_acquire))
        {
            // ---- Per-module silence gate --------------------------------
            // While input is silent, keep processing until the module's tail
            // (reported via getTailLengthSamples, sized to ~-120 dB ring-out)
            // plus a 1/4 s pad has flushed through. After that the module's
            // state is inaudible silence and process() can be skipped at zero
            // cost. Any non-silent block re-arms the countdown and resumes
            // processing instantly — the flushed state guarantees no click.
            // The pad keeps rapid pause/play cycles away from the gate edge.
            const int numSamples = buffer.getNumSamples();
            float mag = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                mag = std::max(mag, buffer.getMagnitude(ch, 0, numSamples));

            if (mag < kGateSilenceThreshold)
            {
                if (gateCountdown <= 0)
                    return;                      // fully rung out — skip
                gateCountdown -= numSamples;     // still flushing the tail
            }
            else
            {
                gateCountdown = m->getTailLengthSamples(currentSpec.sampleRate)
                              + (int)(currentSpec.sampleRate * 0.25);
            }

            m->setPlayHead(playHead);
            m->process(buffer, midi);
        }

    }

    void setModule(std::unique_ptr<EffectModule> newModule)
    {
        if (newModule)
        {
            if (currentSpec.sampleRate > 0)
                newModule->prepare(currentSpec);
            newModule->setID(slotID);
        }

        // Keep old module alive until after swap
        pendingDeletion = std::move(ownedModule);
        ownedModule = std::move(newModule);

        // Atomic pointer swap (audio thread safe)
        activeModule.store(ownedModule.get(), std::memory_order_release);
    }

    // Install a module that has ALREADY been prepare()d and setID()d.
    // Does only the pointer swap — no heavy work — so it is safe to call
    // while holding the audio callback lock.
    void installPreparedModule(std::unique_ptr<EffectModule> newModule)
    {
        pendingDeletion = std::move(ownedModule);
        ownedModule     = std::move(newModule);
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

    // Signal convolver threads in every module this slot owns (active + pending).
    // Used by the processor destructor to parallelise thread joins before the
    // sequential TwoStageFFTConvolver destructors run (each waits up to 500 ms).
    template <typename Fn>
    void forEachOwnedModule(Fn&& fn)
    {
        if (ownedModule)     fn(ownedModule.get());
        if (pendingDeletion) fn(pendingDeletion.get());
    }

    // Destroy pendingDeletion on the calling (message) thread, pre-signalling its
    // convolver threads so the join is fast. Never call under the audio lock.
    void retirePending()
    {
        if (auto* cm = dynamic_cast<ConvolutionModule*>(pendingDeletion.get()))
            cm->signalConvolversToStop();
        pendingDeletion.reset();
    }

    // Extract ALL owned modules (active + any pending deletion) into dest for
    // deferred destruction outside the audio lock. After this call the slot is
    // completely empty: activeModule == null, ownedModule == null,
    // pendingDeletion == null. No destructors are run inside this call.
    void extractAllModules(std::vector<std::unique_ptr<EffectModule>>& dest)
    {
        activeModule.store(nullptr, std::memory_order_release);
        if (ownedModule)     dest.push_back(std::move(ownedModule));
        if (pendingDeletion) dest.push_back(std::move(pendingDeletion));
    }

    EffectModule* get() { return ownedModule.get(); }

    juce::String slotID;
    bool bypassed = false;

private:
    juce::dsp::ProcessSpec currentSpec{};

    // Silence-gate state (audio thread only). 0 = safe to skip on silent
    // input (a freshly installed module has all-zero state, so 0 is also the
    // correct value after a swap). Re-armed on every non-silent block.
    static constexpr float kGateSilenceThreshold = 1.0e-6f;   // ~ -120 dBFS
    int gateCountdown = 0;

    std::unique_ptr<EffectModule> ownedModule;
    std::unique_ptr<EffectModule> pendingDeletion;

    std::atomic<EffectModule*> activeModule{ nullptr };
};