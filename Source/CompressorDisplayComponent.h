/*
  ==============================================================================

    CompressorDisplayComponent.h
    Peak meter display for the compressor module.

    Shows two vertical bar meters side by side:
      - Input level (pre-compression, in dBFS)
      - Gain reduction (how much the compressor is attenuating, in dB)

    The threshold is drawn as a horizontal line on the input meter so the
    user can see when the compressor is engaged.

  ==============================================================================
*/

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>
  #include <juce_events/juce_events.h>
  #include <juce_graphics/juce_graphics.h>
  #include <juce_gui_basics/juce_gui_basics.h>
#endif

#include "CompressorModule.h"

class CompressorDisplayComponent : public juce::Component,
                                   private juce::Timer
{
public:
    explicit CompressorDisplayComponent(CompressorModule& modRef);

    // Called by CompressorModuleSlotEditor's timer when meterReady is true
    void pushMeterValues(float inputDb, float grDb);

    void paint(juce::Graphics&) override;

private:
    CompressorModule& mod;

    // Smoothed display values (UI thread only)
    float displayInputDb    = -100.0f;
    float displayGrDb       =    0.0f;

    // Peak hold
    float peakInputDb       = -100.0f;
    float peakGrDb          =    0.0f;
    int   peakInputHold     = 0;
    int   peakGrHold        = 0;

    static constexpr int   peakHoldFrames = 90;   // ~1.5 s at 60 Hz
    static constexpr float meterFloor     = -60.0f;
    static constexpr float meterCeil      =  6.0f;
    static constexpr float grFloor        = -30.0f; // max GR shown

    void timerCallback() override;

    // Draw a single vertical bar meter inside `area`
    // value: current level in dB, peak: peak hold in dB
    // colour: bar colour, label: text drawn below
    // isGR: if true the bar grows downward from the top (reduction)
    void drawMeter(juce::Graphics& g,
                   juce::Rectangle<int> area,
                   float value,
                   float peak,
                   juce::Colour colour,
                   const juce::String& label,
                   bool isGR) const;

    // Map a dB value to a normalised 0..1 position within the meter range
    float dbToNorm(float db, float floor, float ceil) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorDisplayComponent)
};