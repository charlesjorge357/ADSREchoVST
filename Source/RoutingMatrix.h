/*
  ==============================================================================

    RoutingMatrix.h
    Flexible routing and mixing system for effects
    Allows chaining effects in various configurations

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

class RoutingMatrix
{
public:
    enum class EffectSlot
    {
        Delay,
        AlgorithmicReverb,  // Hall or Plate
        ConvolutionReverb,
        Compressor,
        EQ
    };

    struct EffectConnection
    {
        EffectSlot source;
        EffectSlot destination;
        float sendAmount = 1.0f;  // 0-1, allows partial sends
    };

    RoutingMatrix();
    ~RoutingMatrix();


    // Custom routing
    void addConnection(EffectSlot source, EffectSlot dest, float sendAmount = 1.0f);
    void removeConnection(EffectSlot source, EffectSlot dest);
    void clearAllConnections();
    std::vector<EffectConnection> getConnections() const { return connections; }

    // Effect enable/bypass
    void setEffectEnabled(EffectSlot effect, bool enabled);
    bool isEffectEnabled(EffectSlot effect) const;

    // Parallel mixing for reverb blending
    void setReverbBlendRatio(float ratio); // 0 = all algorithmic, 1 = all convolution
    float getReverbBlendRatio() const { return reverbBlendRatio; }

    // Get processing order for current topology
    std::vector<EffectSlot> getProcessingOrder() const;

    // Check if two effects should be processed in parallel
    bool areEffectsParallel(EffectSlot a, EffectSlot b) const;

    // Serialization for preset saving
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);

private:
    std::vector<EffectConnection> connections;
    std::map<EffectSlot, bool> effectEnabled;

    // Reverb blending (for ReverbBlend topology)
    float reverbBlendRatio = 0.5f;


    // Helper to build processing order from connections
    std::vector<EffectSlot> buildProcessingOrder() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingMatrix)
};
