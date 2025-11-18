// HybridPlate.h
#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"  // for Projucer
#else // for Cmake
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

#include "CustomDelays.h"   // DelayLineWithSampleAccess
#include "LFO.h"
#include "ProcessorBase.h"
#include "Utilities.h"

class HybridPlate : public ReverbProcessorBase
{
public:
    HybridPlate();
    ~HybridPlate() override;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void reset() override;

    ReverbProcessorParameters& getParameters() override;
    void setParameters(const ReverbProcessorParameters& params) override;

private:
    // parameter state
    ReverbProcessorParameters parameters;

    // --- short serial diffusers (allpass-ish behaviour). Use small JUCE delays.
    std::vector<juce::dsp::DelayLine<float>> diffusers;
    std::vector<float> diffuserDelayTimes { 60.0f, 75.0f, 90.0f, 110.0f }; // samples (base)

    // --- FDN (4x4) body using your DelayLineWithSampleAccess so it matches Datorro usage
    std::array<DelayLineWithSampleAccess<float>, 4> fdnLines {
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100)
    };
    std::vector<float> fdnDelayTimes { 300.0f, 370.0f, 420.0f, 510.0f }; // base samples

    // damping filters per FDN line (first order TPT)
    std::array<juce::dsp::FirstOrderTPTFilter<float>, 4> dampingFilters;

    // LFO (reuse the same LFO style as your project)
    LFO lfo;
    OscillatorParameters lfoParameters;
    SignalGenData lfoOutput;

    // System constants
    static constexpr int fdnCount = 4;
    static constexpr int diffuserCount = 4;

    // Hadamard-ish feedback matrix for cross-feedback
    static constexpr float feedbackMatrix[fdnCount][fdnCount] = {
        {  0.5f,  0.5f,  0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f,  0.5f }
    };

    // runtime state
    float stereoWidth = 23.0f;
    double sampleRate = 44100.0;
};
