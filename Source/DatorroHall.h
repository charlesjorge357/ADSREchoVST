// ================================================================
// DatorroHall.h - Complete with ML support
// ================================================================
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
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void reset() override;
    
    ReverbProcessorParameters& getParameters() override;
    void setParameters(const ReverbProcessorParameters& params) override;
    
    // ML-specific methods
    void setMLParameters(const MLDatorroParameters& mlParams)
    {
        mlParameters = mlParams;
        mlParameters.clipToSafeRanges();
    }
    
    MLDatorroParameters& getMLParameters() { return mlParameters; }
    const MLDatorroParameters& getMLParameters() const { return mlParameters; }
    
    static constexpr size_t getMLParameterCount() { return MLDatorroParameters::getParameterCount(); }

private:    
    // Standard parameters
    ReverbProcessorParameters parameters;
    
    // ML-controllable parameters
    MLDatorroParameters mlParameters;
    
    // Base constants
    static constexpr float baseLoopDelay = 266.0f;
    
    // Filters
    juce::dsp::DelayLine<float> inputBandwidth { 4 };
    juce::dsp::DelayLine<float> feedbackDamping { 4 };
    juce::dsp::FirstOrderTPTFilter<float> loopDamping;
    
    // Chorus allpass
    juce::dsp::DelayLine<float> allpassChorusL { 1764 };
    juce::dsp::DelayLine<float> allpassChorusR { 1764 };
    
    // Input delay
    juce::dsp::DelayLine<float> inputZ { 4 };
    
    // Main delay lines (L)
    DelayLineWithSampleAccess<float> loopDelayL1 { 8 };
    DelayLineWithSampleAccess<float> loopDelayL2 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayL3 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayL4 { 4410 };
    
    // Main delay lines (R)
    DelayLineWithSampleAccess<float> loopDelayR1 { 8 };
    DelayLineWithSampleAccess<float> loopDelayR2 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayR3 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayR4 { 4410 };
    
    // Allpass filters (L)
    juce::dsp::DelayLine<float> allpassL1 { 4410 };
    juce::dsp::DelayLine<float> allpassL2 { 4410 };
    juce::dsp::DelayLine<float> allpassL3Inner { 4410 };
    juce::dsp::DelayLine<float> allpassL3Outer { 4410 };
    juce::dsp::DelayLine<float> allpassL4Innermost { 4410 };
    juce::dsp::DelayLine<float> allpassL4Inner { 4410 };
    juce::dsp::DelayLine<float> allpassL4Outer { 4410 };
    
    // Allpass filters (R)
    juce::dsp::DelayLine<float> allpassR1 { 4410 };
    juce::dsp::DelayLine<float> allpassR2 { 4410 };
    juce::dsp::DelayLine<float> allpassR3Inner { 4410 };
    juce::dsp::DelayLine<float> allpassR3Outer { 4410 };
    juce::dsp::DelayLine<float> allpassR4Innermost { 4410 };
    juce::dsp::DelayLine<float> allpassR4Inner { 4410 };
    juce::dsp::DelayLine<float> allpassR4Outer { 4410 };
    
    // LFO
    OscillatorParameters lfoParameters;
    SignalGenData lfoOutput;
    LFO lfo;
    
    // Processing state
    float allpassOutputInnermost = 0;
    float allpassOutputInner = 0;
    float allpassOutputOuter = 0;
    float feedforwardInnermost = 0;
    float feedforwardInner = 0;
    float feedforwardOuter = 0;
    float feedbackInnermost = 0;
    float feedbackInner = 0;
    float feedbackOuter = 0;
    
    std::vector<float> channelInput {0, 0};
    std::vector<float> channelFeedback {0, 0};
    std::vector<float> channelOutput {0, 0};
    
    int sampleRate = 44100;
};