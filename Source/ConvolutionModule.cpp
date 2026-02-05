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
}

void ConvolutionModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    convolutionReverb.prepare(spec);
}

void ConvolutionModule::process(juce::AudioBuffer<float>& buffer,
                                juce::MidiBuffer& midi)
{
    // Build parameter struct from APVTS
    ConvolutionParameters params;

    params.mix       = state.getRawParameterValue(moduleID + ".mix")->load();
    params.preDelay  = state.getRawParameterValue(moduleID + ".pre delay")->load();

    // Convolution-specific controls
    params.irIndex   = state.getRawParameterValue(moduleID + ".conv ir index")->load();
    params.irGainDb  = state.getRawParameterValue(moduleID + ".conv ir gain")->load();
    params.lowCutHz  = state.getRawParameterValue(moduleID + ".conv low cut")->load();
    params.highCutHz = state.getRawParameterValue(moduleID + ".conv high cut")->load();

    convolutionReverb.setParameters(params);

    // Enabled flag follows same pattern as DatorroModule
    if (*state.getRawParameterValue(moduleID + ".enabled") == true)
        convolutionReverb.processBlock(buffer, midi);
}

std::vector<juce::String> ConvolutionModule::getUsedParameters() const
{
    // Same style as DatorroModule: param *names* only, no prefix
    return {
        "mix",
        "pre delay",
        "conv ir index",
        "conv ir gain",
        "conv low cut",
        "conv high cut"
    };
}

void ConvolutionModule::setID(juce::String& newID)
{
    moduleID = newID;
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
