// Datorro Hall

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

//#include "DelayLineWithSampleAccess.h"
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
private:    
    // parameter class
    ReverbProcessorParameters parameters;
    
    // filters
    juce::dsp::DelayLine<float> inputBandwidth { 4 };
    juce::dsp::DelayLine<float> feedbackDamping { 4 };
    juce::dsp::FirstOrderTPTFilter<float> loopDamping;
    // L
    juce::dsp::DelayLine<float> allpassChorusL { 1764 };
    // R
    juce::dsp::DelayLine<float> allpassChorusR { 1764 };

    // delays
    juce::dsp::DelayLine<float> inputZ { 4 };
    // L
    DelayLineWithSampleAccess<float> loopDelayL1 { 8 };
    DelayLineWithSampleAccess<float> loopDelayL2 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayL3 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayL4 { 4410 };
    // R
    DelayLineWithSampleAccess<float> loopDelayR1 { 8 };
    DelayLineWithSampleAccess<float> loopDelayR2 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayR3 { 4410 };
    DelayLineWithSampleAccess<float> loopDelayR4 { 4410 };

    // allpasses
    
    // L
    juce::dsp::DelayLine<float> allpassL1 { 4410 };
    juce::dsp::DelayLine<float> allpassL2 { 4410 };
    juce::dsp::DelayLine<float> allpassL3Inner { 4410 };
    juce::dsp::DelayLine<float> allpassL3Outer { 4410 };
    juce::dsp::DelayLine<float> allpassL4Innermost { 4410 };
    juce::dsp::DelayLine<float> allpassL4Inner { 4410 };
    juce::dsp::DelayLine<float> allpassL4Outer { 4410 };

    // R
    juce::dsp::DelayLine<float> allpassR1 { 4410 };
    juce::dsp::DelayLine<float> allpassR2 { 4410 };
    juce::dsp::DelayLine<float> allpassR3Inner { 4410 };
    juce::dsp::DelayLine<float> allpassR3Outer { 4410 };
    juce::dsp::DelayLine<float> allpassR4Innermost { 4410 };
    juce::dsp::DelayLine<float> allpassR4Inner { 4410 };
    juce::dsp::DelayLine<float> allpassR4Outer { 4410 };

    OscillatorParameters lfoParameters;
    SignalGenData lfoOutput;
    LFO lfo;

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