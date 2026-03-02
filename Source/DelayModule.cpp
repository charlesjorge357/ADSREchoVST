/*
  ==============================================================================
    DelayModule.cpp
    Effect module for delay
  ==============================================================================
*/

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>
#endif

#include "DelayModule.h"

DelayModule::DelayModule(const juce::String& id, juce::AudioProcessorValueTreeState& apvts)
    : moduleID(id), state(apvts) {}

void DelayModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    delay.prepare(spec);
}

void DelayModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    delay.setMix     (*state.getRawParameterValue(moduleID + ".mix"));
    delay.setFeedback(*state.getRawParameterValue(moduleID + ".feedback"));

    const bool syncEnabled =
        state.getRawParameterValue(moduleID + ".delaySyncEnabled")->load() > 0.5f;

    if (syncEnabled)
    {
        // Resolve BPM: prefer host transport, fall back to manual parameter
        float bpm = state.getRawParameterValue(moduleID + ".delayBpm")->load();
        if (playHead)
        {
            if (auto posInfo = playHead->getPosition())
                if (auto hostBpm = posInfo->getBpm())
                    bpm = static_cast<float>(*hostBpm);
        }

        // Map the APVTS int index to BasicDelay::SyncDivision.
        // The APVTS parameter uses 14 divisions (includes 1/32, dotted 1/16,
        // triplet 1/16).  BasicDelay::SyncDivision covers 11 of those.
        // Indices that have no matching enum value fall back to Quarter (index 2).
        //
        // APVTS index -> SyncDivision mapping:
        //   0  Whole            -> Whole          (0)
        //   1  Half             -> Half           (1)
        //   2  Quarter          -> Quarter        (2)
        //   3  Eighth           -> Eighth         (3)
        //   4  Sixteenth        -> Sixteenth      (4)
        //   5  ThirtySecond     -> (none) -> Quarter fallback
        //   6  DottedHalf       -> DottedHalf     (5)
        //   7  DottedQuarter    -> DottedQuarter  (6)
        //   8  DottedEighth     -> DottedEighth   (7)
        //   9  DottedSixteenth  -> (none) -> Quarter fallback
        //  10  TripletHalf      -> TripletHalf    (8)
        //  11  TripletQuarter   -> TripletQuarter (9)
        //  12  TripletEighth    -> TripletEighth  (10)
        //  13  TripletSixteenth -> (none) -> Quarter fallback
        static const int kDivisionMap[14] =
        {
            0,   // Whole
            1,   // Half
            2,   // Quarter
            3,   // Eighth
            4,   // Sixteenth
            2,   // ThirtySecond -> fallback Quarter
            5,   // DottedHalf
            6,   // DottedQuarter
            7,   // DottedEighth
            2,   // DottedSixteenth -> fallback Quarter
            8,   // TripletHalf
            9,   // TripletQuarter
            10,  // TripletEighth
            2,   // TripletSixteenth -> fallback Quarter
        };

        const int rawIndex = static_cast<int>(
            state.getRawParameterValue(moduleID + ".delayNoteDiv")->load());

        const int safeIndex = (rawIndex >= 0 && rawIndex < 14) ? rawIndex : 2;
        const auto division = static_cast<BasicDelay::SyncDivision>(kDivisionMap[safeIndex]);

        // Pass BPM + division directly into BasicDelay so its internal smoother
        // glides to the new delay time rather than snapping.
        delay.setBpmSync(true, bpm, division);
    }
    else
    {
        // Free-running ms delay -- smoother handles the glide
        delay.setDelayTime(
            state.getRawParameterValue(moduleID + ".delayTime")->load());
    }

    const int modeChoice = static_cast<int>(
        state.getRawParameterValue(moduleID + ".delayMode")->load());
    delay.setMode(static_cast<BasicDelay::DelayMode>(modeChoice));
    delay.setPan         (*state.getRawParameterValue(moduleID + ".delayPan"));
    delay.setLowpassFreq (*state.getRawParameterValue(moduleID + ".delayLowpass"));
    delay.setHighpassFreq(*state.getRawParameterValue(moduleID + ".delayHighpass"));

    if (*state.getRawParameterValue(moduleID + ".enabled") > 0.5f)
        delay.processBlock(buffer);
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

void DelayModule::setID(juce::String& newID)              { moduleID = newID; }
void DelayModule::setPlayHead(juce::AudioPlayHead* ph)    { playHead = ph; }
juce::String DelayModule::getID()   const                 { return moduleID; }
juce::String DelayModule::getType() const                 { return "Delay"; }