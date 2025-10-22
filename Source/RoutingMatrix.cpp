/*
  ==============================================================================

    RoutingMatrix.cpp
    Flexible routing and mixing system for effects

  ==============================================================================
*/

#include "RoutingMatrix.h"

RoutingMatrix::RoutingMatrix()
{
    setTopology(RoutingTopology::Classic);
}

RoutingMatrix::~RoutingMatrix()
{
}

void RoutingMatrix::setTopology(RoutingTopology topology)
{
    currentTopology = topology;
    updateProcessingOrder();
}

void RoutingMatrix::updateProcessingOrder()
{
    processingOrder.clear();

    switch (currentTopology)
    {
        case RoutingTopology::Classic:
            // EQ → Comp → Delay → AlgReverb → ConvReverb
            processingOrder = {
                EffectSlot::EQ,
                EffectSlot::Compressor,
                EffectSlot::Delay,
                EffectSlot::AlgorithmicReverb,
                EffectSlot::ConvolutionReverb
            };
            break;

        case RoutingTopology::ReverbBlend:
            // Delay → (AlgReverb + ConvReverb parallel) → EQ
            processingOrder = {
                EffectSlot::Delay,
                EffectSlot::AlgorithmicReverb,
                EffectSlot::ConvolutionReverb,
                EffectSlot::EQ
            };
            break;

        case RoutingTopology::DelayToConvolution:
            // EQ → Comp → Delay → ConvReverb → AlgReverb
            processingOrder = {
                EffectSlot::EQ,
                EffectSlot::Compressor,
                EffectSlot::Delay,
                EffectSlot::ConvolutionReverb,
                EffectSlot::AlgorithmicReverb
            };
            break;

        case RoutingTopology::ConvolutionFirst:
            // ConvReverb → Delay → AlgReverb → EQ → Comp
            processingOrder = {
                EffectSlot::ConvolutionReverb,
                EffectSlot::Delay,
                EffectSlot::AlgorithmicReverb,
                EffectSlot::EQ,
                EffectSlot::Compressor
            };
            break;

        case RoutingTopology::ParallelProcessing:
            // All effects in parallel
            processingOrder = {
                EffectSlot::EQ,
                EffectSlot::Compressor,
                EffectSlot::Delay,
                EffectSlot::AlgorithmicReverb,
                EffectSlot::ConvolutionReverb
            };
            break;

        case RoutingTopology::Custom:
            // User-defined order (default to Classic for now)
            processingOrder = {
                EffectSlot::EQ,
                EffectSlot::Compressor,
                EffectSlot::Delay,
                EffectSlot::AlgorithmicReverb,
                EffectSlot::ConvolutionReverb
            };
            break;
    }
}

std::vector<RoutingMatrix::EffectSlot> RoutingMatrix::getProcessingOrder() const
{
    return processingOrder;
}

bool RoutingMatrix::isEffectEnabled(EffectSlot slot) const
{
    // For now, all effects are enabled
    // TODO: Implement per-effect enable/disable
    juce::ignoreUnused(slot);
    return true;
}

RoutingMatrix::RoutingTopology RoutingMatrix::getCurrentTopology() const
{
    return currentTopology;
}

void RoutingMatrix::setReverbBlendRatio(float ratio)
{
    reverbBlend = juce::jlimit(0.0f, 1.0f, ratio);
}

float RoutingMatrix::getReverbBlendRatio() const
{
    return reverbBlend;
}
