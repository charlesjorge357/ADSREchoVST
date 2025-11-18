// ================================================================
// HybridPlate.h - Complete with ML support
// ================================================================
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

#include "CustomDelays.h"
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
    
    // ML-specific methods
    void setMLParameters(const MLReverbParameters& mlParams) 
    { 
        mlParameters = mlParams;
        mlParameters.clipToSafeRanges();
    }
    
    MLReverbParameters& getMLParameters() { return mlParameters; }
    const MLReverbParameters& getMLParameters() const { return mlParameters; }
    
    static constexpr size_t getMLParameterCount() { return MLReverbParameters::getParameterCount(); }

private:
    // Standard parameters
    ReverbProcessorParameters parameters;
    
    // ML-controllable parameters
    MLReverbParameters mlParameters;
    
    // Base delay times (constants - these never change)
    static constexpr float baseDiffuserDelayTimes[4] = { 60.0f, 75.0f, 90.0f, 110.0f };
    static constexpr float baseFdnDelayTimes[4] = { 300.0f, 370.0f, 420.0f, 510.0f };
    static constexpr float baseStereoWidth = 23.0f;
    
    // Short serial diffusers (allpass-ish behaviour)
    std::vector<juce::dsp::DelayLine<float>> diffusers;
    
    // FDN (4x4) body using DelayLineWithSampleAccess
    std::array<DelayLineWithSampleAccess<float>, 4> fdnLines {
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100)
    };
    
    // Damping filters per FDN line (first order TPT)
    std::array<juce::dsp::FirstOrderTPTFilter<float>, 4> dampingFilters;
    
    // LFO
    LFO lfo;
    OscillatorParameters lfoParameters;
    SignalGenData lfoOutput;
    
    // System constants
    static constexpr int fdnCount = 4;
    static constexpr int diffuserCount = 4;
    
    // Base Hadamard feedback matrix (will be scaled by ML parameters)
    static constexpr float baseFeedbackMatrix[fdnCount][fdnCount] = {
        {  0.5f,  0.5f,  0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f,  0.5f }
    };
    
    // Runtime state
    double sampleRate = 44100.0;
};