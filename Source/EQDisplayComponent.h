/*
  ==============================================================================

    EQDisplayComponent.h
    Created: 2 Mar 2026 3:20:00pm
    Author:  Crabnebuelar

  ==============================================================================
*/

#pragma once

#if __has_include("JuceHeader.h")
#include "JuceHeader.h"
#else
    #include <juce_audio_basics/juce_audio_basics.h>
    #include <juce_audio_formats/juce_audio_formats.h>
    #include <juce_audio_plugin_client/juce_audio_plugin_client.h>
    #include <juce_audio_processors/juce_audio_processors.h>
    #include <juce_audio_utils/juce_audio_utils.h>
    #include <juce_core/juce_core.h>
    #include <juce_data_structures/juce_data_structures.h>
    #include <juce_dsp/juce_dsp.h>
    #include <juce_events/juce_events.h>
    #include <juce_graphics/juce_graphics.h>
    #include <juce_gui_basics/juce_gui_basics.h>
    #include <juce_gui_extra/juce_gui_extra.h>
#endif

#include "EQModule.h"

class EQDisplayComponent : public juce::Component,
    private juce::Timer
{
public:
    EQDisplayComponent(EQModule& modRef);

    void pushSamples(const float* samples, int numSamples);

    void paint(juce::Graphics&) override;

private:
    EQModule& mod;

    // ===== FFT =====
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    float fifo[fftSize] = { 0.0f };
    float fftData[2 * fftSize] = { 0.0f };
    int fifoIndex = 0;
    bool ready = false;

    std::vector<float> smoothedFFT;

    void timerCallback() override;
    void drawNextFrame();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQDisplayComponent)
};