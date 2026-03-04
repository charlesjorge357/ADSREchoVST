/*
  ==============================================================================

    ConvolutionModule.cpp
    Effect module for convolution reverb

  ==============================================================================
*/

#include "ConvolutionModule.h"

ConvolutionModule::ConvolutionModule(const juce::String& id,
                                     juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts)
{
    rebuildParamIDs();
}

void ConvolutionModule::rebuildParamIDs()
{
    // Build all parameter ID strings once so process() never allocates
    pMix      = moduleID + ".mix";
    pPreDelay = moduleID + ".preDelay";
    pIrIndex  = moduleID + ".convIrIndex";
    pIrGain   = moduleID + ".convIrGain";
    pLowCut   = moduleID + ".convLowCut";
    pHighCut  = moduleID + ".convHighCut";
    pEnabled  = moduleID + ".enabled";
}

void ConvolutionModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    convolutionReverb.prepare(spec);
}

void ConvolutionModule::process(juce::AudioBuffer<float>& buffer,
                                juce::MidiBuffer& midi)
{
    ConvolutionParameters params;

    params.mix       = state.getRawParameterValue(pMix)     ->load();
    params.preDelay  = state.getRawParameterValue(pPreDelay) ->load();
    params.irIndex   = (int)state.getRawParameterValue(pIrIndex)  ->load();
    params.irGainDb  = state.getRawParameterValue(pIrGain)   ->load();
    params.lowCutHz  = state.getRawParameterValue(pLowCut)   ->load();
    params.highCutHz = state.getRawParameterValue(pHighCut)  ->load();

    convolutionReverb.setParameters(params);

    if (*state.getRawParameterValue(pEnabled) > 0.5f)
        convolutionReverb.processBlock(buffer, midi);
}

std::vector<juce::String> ConvolutionModule::getUsedParameters() const
{
    return {
        "mix",
        "preDelay",
        "convIrIndex",
        "convIrGain",
        "convLowCut",
        "convHighCut"
    };
}

void ConvolutionModule::setID(juce::String& newID)
{
    moduleID = newID;
    rebuildParamIDs(); // Keep cached IDs in sync
}

juce::String ConvolutionModule::getID() const
{
    return moduleID;
}

juce::String ConvolutionModule::getType() const
{
    return "Convolution";
}

void ConvolutionModule::setIRBank(std::shared_ptr<IRBank> bank)
{
    convolutionReverb.setIRBank(bank);
}

void ConvolutionModule::loadCustomIR(const juce::File& file)
{
    convolutionReverb.loadCustomIR(file);
}

void ConvolutionModule::clearCustomIR()
{
    convolutionReverb.clearCustomIR();
}

bool ConvolutionModule::hasCustomIR() const
{
    return convolutionReverb.hasCustomIR();
}

juce::String ConvolutionModule::getCustomIRPath() const
{
    return convolutionReverb.getCustomIRPath();
}