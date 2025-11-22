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
    // Current parameter state
    ReverbProcessorParameters parameters;

    // Tank damping (high-cut in the feedback loop)
    juce::dsp::FirstOrderTPTFilter<float> loopDamping;

    // Main tank delay lines (per channel)
    DelayLineWithSampleAccess<float> loopDelayL { 44100 }; // up to ~1s @ 44.1k
    DelayLineWithSampleAccess<float> loopDelayR { 44100 };

    float currentDelayL_samps = 0.0f;
    float currentDelayR_samps = 0.0f;

    float estimatedLoopTimeSeconds = 0.2f; // default safety value


    // Early diffusion allpasses (per channel)
    Allpass<float> earlyL1;
    Allpass<float> earlyL2;
    Allpass<float> earlyR1;
    Allpass<float> earlyR2;

    // Late/tank diffusion allpasses (per channel)
    Allpass<float> tankL1;
    Allpass<float> tankL2;
    Allpass<float> tankR1;
    Allpass<float> tankR2;

    // LFO for delay modulation
    OscillatorParameters lfoParameters;
    SignalGenData       lfoOutput;
    LFO                 lfo;

    // Per-channel I/O and feedback
    std::vector<float> channelInput   { 0.0f, 0.0f };
    std::vector<float> channelFeedback{ 0.0f, 0.0f };
    std::vector<float> channelOutput  { 0.0f, 0.0f };

    // Base tank delays and limits (in samples)
    float baseDelaySamplesL = 0.0f;
    float baseDelaySamplesR = 0.0f;
    float maxDelaySamplesL  = 0.0f;
    float maxDelaySamplesR  = 0.0f;

    int sampleRate = 44100;

    // Helpers
    void prepareAllpass(Allpass<float>& ap,
                        const juce::dsp::ProcessSpec& spec,
                        float delayMs,
                        float gain);

    void updateInternalParamsFromUserParams();
};
