/*
  ==============================================================================

    EQModule.cpp
    Effect module for 3-band parametric EQ

  ==============================================================================
*/

#include "EQModule.h"

EQModule::EQModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts)
{
}

void EQModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    eq.prepare(spec);
}

void EQModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    // Low shelf
    eq.setLowFreq    (state.getRawParameterValue(moduleID + ".eqLowFreq") ->load());
    eq.setLowGain    (state.getRawParameterValue(moduleID + ".eqLowGain") ->load());
    eq.setLowQ       (state.getRawParameterValue(moduleID + ".eqLowQ")    ->load());

    // Mid peak
    eq.setMidFreq    (state.getRawParameterValue(moduleID + ".eqMidFreq") ->load());
    eq.setMidGain    (state.getRawParameterValue(moduleID + ".eqMidGain") ->load());
    eq.setMidQ       (state.getRawParameterValue(moduleID + ".eqMidQ")    ->load());

    // High shelf
    eq.setHighFreq   (state.getRawParameterValue(moduleID + ".eqHighFreq")->load());
    eq.setHighGain   (state.getRawParameterValue(moduleID + ".eqHighGain")->load());
    eq.setHighQ      (state.getRawParameterValue(moduleID + ".eqHighQ")   ->load());

    if (*state.getRawParameterValue(moduleID + ".enabled") > 0.5f)
        eq.processBlock(buffer);



    auto* samples = buffer.getReadPointer(0);

    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        fftBuffer[fftIndex++] = samples[i];

        if (fftIndex == fftSize)
        {
            fftReady = true;
            fftIndex = 0;
        }
    }
}

std::vector<juce::String> EQModule::getUsedParameters() const
{
    return {
        "eqLowFreq",  "eqLowGain",  "eqLowQ",
        "eqMidFreq",  "eqMidGain",  "eqMidQ",
        "eqHighFreq", "eqHighGain", "eqHighQ"
    };
}

void EQModule::setID(juce::String& newID) { moduleID = newID; }

float EQModule::getMagnitudeForFrequency(float freq) {
    return eq.getMagnitudeForFrequency(freq);
}

float EQModule::getSampleRate() {
    return eq.sampleRate;
}

juce::String EQModule::getID()   const { return moduleID; }
juce::String EQModule::getType() const { return "EQ"; }