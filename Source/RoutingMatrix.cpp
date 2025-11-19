/*
  ==============================================================================

    RoutingMatrix.cpp
    Flexible routing and mixing system for effects

  ==============================================================================
*/

#include "RoutingMatrix.h"

RoutingMatrix::RoutingMatrix()
{
    // Initialize all effects as enabled
    effectEnabled[EffectSlot::Delay] = true;
    effectEnabled[EffectSlot::AlgorithmicReverb] = true;
    effectEnabled[EffectSlot::ConvolutionReverb] = true;
    effectEnabled[EffectSlot::EQ] = true;
    effectEnabled[EffectSlot::Compressor] = true;
}

RoutingMatrix::~RoutingMatrix()
{
}



std::vector<RoutingMatrix::EffectSlot> RoutingMatrix::getProcessingOrder() const
{
    return buildProcessingOrder();
}

std::vector<RoutingMatrix::EffectSlot> RoutingMatrix::buildProcessingOrder() const
{
    // Simple topological sort would go here
    // For now, return a simple default order
    std::vector<EffectSlot> order;

    // Add all effects that are enabled
    if (isEffectEnabled(EffectSlot::EQ))
        order.push_back(EffectSlot::EQ);
    if (isEffectEnabled(EffectSlot::Compressor))
        order.push_back(EffectSlot::Compressor);
    if (isEffectEnabled(EffectSlot::Delay))
        order.push_back(EffectSlot::Delay);
    if (isEffectEnabled(EffectSlot::AlgorithmicReverb))
        order.push_back(EffectSlot::AlgorithmicReverb);
    if (isEffectEnabled(EffectSlot::ConvolutionReverb))
        order.push_back(EffectSlot::ConvolutionReverb);

    return order;
}

void RoutingMatrix::addConnection(EffectSlot source, EffectSlot dest, float sendAmount)
{
    EffectConnection conn;
    conn.source = source;
    conn.destination = dest;
    conn.sendAmount = sendAmount;
    connections.push_back(conn);
}

void RoutingMatrix::removeConnection(EffectSlot source, EffectSlot dest)
{
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
            [source, dest](const EffectConnection& conn) {
                return conn.source == source && conn.destination == dest;
            }),
        connections.end()
    );
}

void RoutingMatrix::clearAllConnections()
{
    connections.clear();
}

void RoutingMatrix::setEffectEnabled(EffectSlot effect, bool enabled)
{
    effectEnabled[effect] = enabled;
}

bool RoutingMatrix::isEffectEnabled(EffectSlot effect) const
{
    auto it = effectEnabled.find(effect);
    return it != effectEnabled.end() ? it->second : true;
}

void RoutingMatrix::setReverbBlendRatio(float ratio)
{
    reverbBlendRatio = juce::jlimit(0.0f, 1.0f, ratio);
}

bool RoutingMatrix::areEffectsParallel(EffectSlot a, EffectSlot b) const
{
    // Simple check: if both receive from same source, they're parallel
    EffectSlot commonSource = EffectSlot::Delay;
    bool aHasSource = false;
    bool bHasSource = false;

    for (const auto& conn : connections)
    {
        if (conn.destination == a && conn.source == commonSource)
            aHasSource = true;
        if (conn.destination == b && conn.source == commonSource)
            bHasSource = true;
    }

    return aHasSource && bHasSource;
}

juce::ValueTree RoutingMatrix::serialize() const
{
    juce::ValueTree tree("RoutingMatrix");
    //tree.setProperty("topology", static_cast<int>(currentTopology), nullptr);
    tree.setProperty("reverbBlend", reverbBlendRatio, nullptr);

    // TODO: Serialize connections and effect enables
    return tree;
}

void RoutingMatrix::deserialize(const juce::ValueTree& tree)
{
    if (tree.hasType("RoutingMatrix"))
    {
        //currentTopology = static_cast<RoutingTopology>(static_cast<int>(tree.getProperty("topology")));
        reverbBlendRatio = tree.getProperty("reverbBlend");

        // TODO: Deserialize connections and effect enables
    }
}
