/*
  ==============================================================================

    DelayModule.cpp
    Effect module for delay

  ==============================================================================
*/

#include "DelayModule.h"
DelayModule::DelayModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts) {
}

void DelayModule::prepare(const juce::dsp::ProcessSpec & spec)
{
    delay.prepare(spec);
}

void DelayModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    delay.setMix( *state.getRawParameterValue(moduleID + ".mix"));
    delay.setFeedback( *state.getRawParameterValue(moduleID + ".feedback"));

    // Update delay parameters
    bool syncEnabled = state.getRawParameterValue(moduleID + ".delaySyncEnabled")->load() > 0.5f;
    if (syncEnabled)
    {
        float bpm = state.getRawParameterValue(moduleID + ".delayBpm")->load();

        // Use host BPM when available, fall back to manual parameter
        if (playHead)
        {
            if (auto posInfo = playHead->getPosition())
            {
                if (auto hostBpm = posInfo->getBpm())
                    bpm = static_cast<float>(*hostBpm);
            }
        }

        int noteDivision = static_cast<int>(state.getRawParameterValue(moduleID + ".delayNoteDiv")->load());

        // Quarter note duration in ms, then scale by note division multiplier
        static const float noteMultipliers[] = {
            4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f,           // straight: 1/1 to 1/32
            3.0f, 1.5f, 0.75f, 0.375f,                         // dotted: 1/2d to 1/16d
            4.0f / 3.0f, 2.0f / 3.0f, 1.0f / 3.0f, 1.0f / 6.0f // triplet: 1/2t to 1/16t
        };

        float quarterNoteMs = 60000.0f / bpm;
        float multiplier = (noteDivision >= 0 && noteDivision < 14)
            ? noteMultipliers[noteDivision] : 1.0f;
        float syncedTime = juce::jlimit(1.0f, 2000.0f, quarterNoteMs * multiplier);

        delay.setDelayTime(syncedTime);
    }
    else
    {
        delay.setDelayTime(state.getRawParameterValue(moduleID + ".delayTime")->load());
    }

    int modeChoice = static_cast<int>(state.getRawParameterValue(moduleID + ".delayMode")->load());
    delay.setMode(static_cast<BasicDelay::DelayMode>(modeChoice));
    delay.setPan(state.getRawParameterValue(moduleID + ".delayPan")->load());
    delay.setLowpassFreq(state.getRawParameterValue(moduleID + ".delayLowpass")->load());
    delay.setHighpassFreq(state.getRawParameterValue(moduleID + ".delayHighpass")->load());

    if (*state.getRawParameterValue(moduleID + ".enabled") == true) { delay.processBlock(buffer); }

}

std::vector<juce::String> DelayModule::getUsedParameters() const
{
    return {
       "mix",
       "delayTime",
       "feedback",
       "delaySyncEnabled",
       "delayBpm",
       "delayNoteDiv",
       "delayMode",
       "delayPan",
       "delayLowpass",
       "delayHighpass"
    };
}

void DelayModule::setID(juce::String& newID) { moduleID = newID; }

void DelayModule::setPlayHead(juce::AudioPlayHead* playHead) { this->playHead = playHead; }

juce::String DelayModule::getID() const { return moduleID; }
juce::String DelayModule::getType() const { return "Delay"; }