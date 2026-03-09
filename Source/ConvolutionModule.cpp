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
    // Cache raw pointers once so process() does direct atomic loads, not HashMap lookups
    pMix      = state.getRawParameterValue(moduleID + ".mix");
    pPreDelay = state.getRawParameterValue(moduleID + ".preDelay");
    pIrIndex  = state.getRawParameterValue(moduleID + ".convIrIndex");
    pIrGain   = state.getRawParameterValue(moduleID + ".convIrGain");
    pLowCut   = state.getRawParameterValue(moduleID + ".convLowCut");
    pHighCut  = state.getRawParameterValue(moduleID + ".convHighCut");
    pEnabled  = state.getRawParameterValue(moduleID + ".enabled");
}

void ConvolutionModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    convolutionReverb.prepare(spec);
}

void ConvolutionModule::process(juce::AudioBuffer<float>& buffer,
                                juce::MidiBuffer& midi)
{
    if (pEnabled->load() < 0.5f)
        return;

    ConvolutionParameters params;
    params.mix       = pMix->load();
    params.preDelay  = pPreDelay->load();
    params.irIndex   = (int) pIrIndex->load();
    params.irGainDb  = pIrGain->load();
    params.lowCutHz  = pLowCut->load();
    params.highCutHz = pHighCut->load();

    convolutionReverb.setParameters(params);
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

void ConvolutionModule::forceReloadIR(int index)
{
    convolutionReverb.forceLoadIRAtIndex(index);
}

bool ConvolutionModule::isIRMissing() const
{
    return convolutionReverb.isIRMissing();
}