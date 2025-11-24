// Datorro Hall - using custom delay lines and allpasses

#pragma once

#include <JuceHeader.h>

#include "CustomDelays.h"
#include "LFO.h"
#include "ProcessorBase.h"
#include "Utilities.h"
#include "PsychoDamping.h"

class DatorroHall : public ReverbProcessorBase
{
public:
    DatorroHall();
    ~DatorroHall() override;

    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;
    void reset() override;

    ReverbProcessorParameters& getParameters() override;
    void setParameters(const ReverbProcessorParameters& params) override;

private:
    //======================================================================
    // Parameters (user-facing wrapped in ReverbProcessorParameters)
    //======================================================================
    ReverbProcessorParameters parameters;

    //======================================================================
    // Tank damping (high-cut in the feedback loop)
    //======================================================================
    juce::dsp::FirstOrderTPTFilter<float> loopDamping;

    // Per-line damping filters (one for each tank line, L/R)
    juce::dsp::FirstOrderTPTFilter<float> dampingFiltersL[4];
    juce::dsp::FirstOrderTPTFilter<float> dampingFiltersR[4];

    //======================================================================
    //Pre-Delay
    //======================================================================
    
    // Pre-delay (mono-in / stereo-out)
    DelayLineWithSampleAccess<float> preDelayL { 44100 };
    DelayLineWithSampleAccess<float> preDelayR { 44100 };
    float preDelaySamples = 0.0f;   // smoothed


    //======================================================================
    // Tank delay lines - 4-line FDN per channel (bright hall style)
    //
    // Each DelayLineWithSampleAccess is effectively mono; we run separate
    // instances for L/R so we can crossfeed between stereo channels AND
    // between the 4 FDN lines.
    //======================================================================
    DelayLineWithSampleAccess<float> tankDelayL1 { 44100 };
    DelayLineWithSampleAccess<float> tankDelayL2 { 44100 };
    DelayLineWithSampleAccess<float> tankDelayL3 { 44100 };
    DelayLineWithSampleAccess<float> tankDelayL4 { 44100 };

    DelayLineWithSampleAccess<float> tankDelayR1 { 44100 };
    DelayLineWithSampleAccess<float> tankDelayR2 { 44100 };
    DelayLineWithSampleAccess<float> tankDelayR3 { 44100 };
    DelayLineWithSampleAccess<float> tankDelayR4 { 44100 };


    DelayLineWithSampleAccess<float> erL { 44100 };
    DelayLineWithSampleAccess<float> erR { 44100 };

    // Smoothed delay times per FDN line per channel (for modulation)
    float currentDelayL_samps[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    float currentDelayR_samps[4] { 0.0f, 0.0f, 0.0f, 0.0f };

    // Base & max delays per line (in samples), set up in prepare()
    float baseDelaySamplesL[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    float baseDelaySamplesR[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    float maxDelaySamplesL[4]  { 0.0f, 0.0f, 0.0f, 0.0f };
    float maxDelaySamplesR[4]  { 0.0f, 0.0f, 0.0f, 0.0f };

    // Estimated loop time for RT60 mapping (seconds)
    float estimatedLoopTimeSeconds = 0.2f; // default safety value

    //======================================================================
    // Early diffusion: 4 allpasses per channel (higher echo density)
    //======================================================================
    Allpass<float> earlyL1;
    Allpass<float> earlyL2;
    Allpass<float> earlyL3;
    Allpass<float> earlyL4;

    Allpass<float> earlyR1;
    Allpass<float> earlyR2;
    Allpass<float> earlyR3;
    Allpass<float> earlyR4;

    //======================================================================
    // Late/tank diffusion: 4 allpasses per channel
    // (can be placed inside tank lines or at tank outputs)
    //======================================================================
    Allpass<float> tankLAP1;
    Allpass<float> tankLAP2;
    Allpass<float> tankLAP3;
    Allpass<float> tankLAP4;

    Allpass<float> tankRAP1;
    Allpass<float> tankRAP2;
    Allpass<float> tankRAP3;
    Allpass<float> tankRAP4;

    // Psycho Filters
    PsychoDamping::OnePole extraDampingL[4];
    PsychoDamping::OnePole extraDampingR[4];


    //======================================================================
    // LFO for modulation of tank delay times (per-line modulation)
    //======================================================================
    OscillatorParameters lfoParameters;
    SignalGenData       lfoOutput;
    LFO                 lfo;

    //======================================================================
    // Per-channel I/O and feedback accumulation
    //======================================================================
    std::vector<float> channelInput    { 0.0f, 0.0f };
    std::vector<float> channelOutput   { 0.0f, 0.0f };

    // Feedback per FDN line per channel (4 lines x 2 channels)
    float feedbackL[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    float feedbackR[4] { 0.0f, 0.0f, 0.0f, 0.0f };

    // Early Reflections (simple 6-tap stereo cluster)
    static constexpr int ER_count = 6;

    float ER_gains[ER_count] =
    {
        0.60f,  // tap 1
        0.45f,  // tap 2
        0.32f,  // tap 3
        0.28f,  // tap 4
        0.22f,  // tap 5
        0.18f   // tap 6
    };

    // Times in milliseconds
    float ER_tapTimesMsLeft[ER_count]  = { 5.2f,  12.8f,  21.5f,  32.2f,  45.0f,  60.0f };
    float ER_tapTimesMsRight[ER_count] = { 7.9f,  17.3f,  25.8f,  37.1f,  48.6f,  64.0f };

    // Converted to samples (computed in prepare)
    float ER_tapSamplesLeft[ER_count];
    float ER_tapSamplesRight[ER_count];


    int sampleRate = 44100;

    //======================================================================
    // Helpers
    //======================================================================
    void prepareAllpass(Allpass<float>& ap,
                        const juce::dsp::ProcessSpec& spec,
                        float delayMs,
                        float gain);

    void updateInternalParamsFromUserParams();

    // Apply a simple 4x4 Householder (or other) scattering matrix
    // to the 4 tank lines for one channel. This is where we can emulate
    // the bright, dense hall behavior similar to Valhalla Vintage Verb.
    void applyFDNScattering(const float in[4], float (&out)[4]) const;
};
