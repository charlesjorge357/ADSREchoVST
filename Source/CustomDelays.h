/*
Tapped delay line, Allpass classes
Delay based on juce::dsp::DelayLine, but allows access to the underlying buffer at specified sample offsets for multiple-tap delays.
*/

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
// #include "Utilities.h"

template <typename SampleType>
class DelayLineWithSampleAccess
{
public:
    DelayLineWithSampleAccess(int maximumDelayInSamples);
    
    ~DelayLineWithSampleAccess();
    
    void pushSample(int channel, SampleType newValue);
    
    SampleType popSample(int channel);
    
    SampleType getSampleAtDelay(int channel, int delay) const;
    
    void setDelay(int newLength);
    
    void setSize(const int numChannels, const int newSize);
    
    int getNumSamples() const;
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    
    void reset();
private:
    juce::AudioBuffer<SampleType> delayBuffer;
    std::vector<SampleType> v;
    int numSamples = 0;
    std::vector<int> writePosition, readPosition;
    SampleType delay = 0.0, delayFrac = 0.0;
    int delayInSamples = 0;
    int totalSize = 4;
    
    double sampleRate = 44100.0;
};

//============================================================================

template <typename SampleType>
class Allpass
{
public:
    Allpass();
    
    ~Allpass();
    
    void setMaximumDelayInSamples(int maxDelayInSamples);
    
    void setDelay(SampleType newDelayInSamples);
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    
    void reset();
    
    void pushSample(int channel, SampleType sample);
    
    SampleType popSample(int channel, SampleType delayInSamples=-1, bool updateReadPointer=true);
    
    void setGain(SampleType newGain);
    
private:
    juce::dsp::DelayLine<SampleType> delayLine { 44100 };
    
    int delayInSamples = 4;
    
    SampleType gain = 0.5;
    
    std::vector<SampleType> drySample { };
    std::vector<SampleType> delayOutput { };
    std::vector<SampleType> feedforward { };
    std::vector<SampleType> feedback { };
    
    SampleType sampleRate = 44100.0;
};