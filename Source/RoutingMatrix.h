/*
  ==============================================================================

    RoutingMatrix.h
    Flexible routing and mixing system for effects
    Allows chaining effects in various configurations

  ==============================================================================
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>

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
