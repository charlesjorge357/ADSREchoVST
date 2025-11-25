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

#include "CustomDelays.h"   // DelayLineWithSampleAccess, Allpass
#include "LFO.h"
#include "ProcessorBase.h"
#include "Utilities.h"
#include "PsychoDamping.h"

class HybridPlate : public ReverbProcessorBase
{
public:
    HybridPlate();
    ~HybridPlate() override;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;
    void reset() override;

    ReverbProcessorParameters& getParameters() override;
    void setParameters(const ReverbProcessorParameters& params) override;

private:
    //======================================================================
    // Parameters
    //======================================================================
    ReverbProcessorParameters parameters;

    // New psycho damping filters for each tank line
     PsychoOnePole extraDampL[4];
     PsychoOnePole extraDampR[4];


    //======================================================================
    // Pre-delay (stereo, using your custom delay line)
    //======================================================================
    DelayLineWithSampleAccess<float> preDelayL { 48000 };  // ~1s @ 48k
    DelayLineWithSampleAccess<float> preDelayR { 48000 };
    float preDelaySamples = 0.0f;                          // in samples

    //======================================================================
    // Early diffusion: 4 allpasses per channel
    //======================================================================
    Allpass<float> earlyL[4];
    Allpass<float> earlyR[4];

    //======================================================================
    // FDN core: 4 delay lines (mono FDN, stereo decode)
    //======================================================================
    static constexpr int fdnCount = 4;
    juce::dsp::IIR::Filter<float> highShelfFilters[fdnCount];

    DelayLineWithSampleAccess<float> fdnLines[fdnCount] = {
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100),
        DelayLineWithSampleAccess<float>(44100)
    };

    float baseDelaySamples[fdnCount]    { 0.f, 0.f, 0.f, 0.f };
    float maxDelaySamples[fdnCount]     { 0.f, 0.f, 0.f, 0.f };
    float currentDelaySamples[fdnCount] { 0.f, 0.f, 0.f, 0.f };

    juce::dsp::FirstOrderTPTFilter<float> dampingFilters[fdnCount];

    float estimatedLoopTimeSeconds = 0.2f;

    //======================================================================
    // LFO for FDN modulation
    //======================================================================
    OscillatorParameters lfoParameters;
    SignalGenData       lfoOutput;
    LFO                 lfo;

    //======================================================================
    // Internal buffers / state
    //======================================================================
    std::vector<float> channelInput  { 0.0f, 0.0f };
    std::vector<float> channelOutput { 0.0f, 0.0f };

    int sampleRate = 44100;

    //======================================================================
    // Feedback matrix (Hadamard-ish) for plate FDN
    //======================================================================
    static constexpr float feedbackMatrix[fdnCount][fdnCount] = {
    {  0.5f,  0.5f,  0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f, -0.5f },
    {  0.5f,  0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f, -0.5f,  0.5f }
    };

    // For an orthonormal matrix, you generally want this = 1.0f
    static constexpr float feedbackMatrixScale = 1.0f; 

    //======================================================================
    // Helpers
    //======================================================================
    void prepareAllpass(Allpass<float>& ap,
                        const juce::dsp::ProcessSpec& spec,
                        float delayMs,
                        float gain);

    void updateInternalParamsFromUserParams();

    void applyFDNFeedbackMatrix(const float in[fdnCount],
                                float (&out)[fdnCount]) const;
};
