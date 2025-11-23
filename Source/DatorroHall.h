// Datorro Hall - using custom delay lines and allpasses

#pragma once

#include <JuceHeader.h>

#include "CustomDelays.h"
#include "LFO.h"
#include "ProcessorBase.h"
#include "Utilities.h"

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

    //======================================================================
    //Pre-Delay
    //======================================================================
    
    // Pre-delay (mono-in / stereo-out)
    DelayLineWithSampleAccess<float> preDelayL { 48000 };
    DelayLineWithSampleAccess<float> preDelayR { 48000 };
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
