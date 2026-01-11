// Miscellaneous functions and classes

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

inline float scale(float input, float inLow, float inHi, float outLow, float outHi)
{
    float scaleFactor = (outHi - outLow)/(inHi - inLow);
    float offset = outLow - inLow;
    return (input * scaleFactor) + offset;
}

inline int wrapInt(int a, int b)
{
    int c = a % b;
    return (c < 0) ? c + b : c;
}

struct ReverbProcessorParameters
{
    ReverbProcessorParameters() {}
    
    ReverbProcessorParameters& operator=(const ReverbProcessorParameters& params)
    {
        if (this != &params)
        {
            damping = params.damping;
            decayTime = params.decayTime;
            diffusion = params.diffusion;
            modDepth = params.modDepth;
            modRate = params.modRate;
            roomSize = params.roomSize;
            mix = params.mix;
            inputBandwidth = params.inputBandwidth;
            preDelay = params.preDelay;
        }
        return *this;
    }
    
    bool operator==(const ReverbProcessorParameters& params) const
    {
        if (params.damping == damping &&
            params.decayTime == decayTime &&
            params.diffusion == diffusion &&
            params.modDepth == modDepth &&
            params.modRate == modRate &&
            params.roomSize == roomSize &&
            params.mix == mix &&
            params.inputBandwidth == inputBandwidth &&
            params.preDelay == preDelay)
            return true;
        
        return false;
    }
    
    float damping = 20000.0f;
    float decayTime = 0.35f;
    float diffusion = 0.75f;
    float modDepth = 0;
    float modRate = 0.35f;
    float roomSize = 1.0f;
    float mix = 0.5f;
    float inputBandwidth = 1.0f;
    float preDelay       = 0.0f;   // 0–200 ms typical
};

struct SlotInfo
{
    juce::String slotID;
    juce::String moduleType;
    std::vector<juce::String> usedParameters;
};

enum class ModuleType
{
    Dattorro,
    HybridPlate,
    Delay
};

//template <typename SampleType>
//class Allpass
//{
//public:
//    Allpass() = default;
//    
//    ~Allpass() = default;
//    
//    void setMaximumDelayInSamples(int maxDelayInSamples)
//    {
//        mDelayLine.setMaximumDelayInSamples(maxDelayInSamples);
//    }
//    
//    void setDelay(SampleType newDelayInSamples)
//    {
//        mDelayLine.setDelay(newDelayInSamples);
//    }
//    
//    void prepare(const juce::dsp::ProcessSpec& spec)
//    {
//        jassert(spec.numChannels > 0);
//        
//        mSampleRate = spec.sampleRate;
//        
//        mDelayLine.prepare(spec);
//        mDampingFilter.prepare(spec);
//        
//        mDampingFilter.setCutoffFrequency(mDamping);
//        
//        drySample.resize(spec.numChannels);
//        delayOutput.resize(spec.numChannels);
//        feedforward.resize(spec.numChannels);
//        feedback.resize(spec.numChannels);
//        
//        std::fill(drySample.begin(), drySample.end(), 0.0);
//        std::fill(delayOutput.begin(), delayOutput.end(), 0.0);
//        std::fill(feedforward.begin(), feedforward.end(), 0.0);
//        std::fill(feedback.begin(), feedback.end(), 0.0);
//        
//        reset();
//    }
//    
//    void reset()
//    {
//        mDelayLine.reset();
//    }
//    
//    void pushSample(int channel, SampleType sample)
//    {
//        mDelayLine.pushSample(channel, sample + feedback[channel]);
//        drySample[channel] = sample;
//    }
//    
//    SampleType popSample(int channel, SampleType delayInSamples=-1, bool updateReadPointer=true)
//    {
//        delayOutput[channel] = mDelayLine.popSample(channel, delayInSamples, updateReadPointer);
//        
////        feedback[channel] = mDampingFilter.processSample(channel, delayOutput[channel] * mGain);
//        feedback[channel] = delayOutput[channel] * mGain;
//        
//        feedforward[channel] = -drySample[channel] - delayOutput[channel] * mGain;
//        
//        return delayOutput[channel] + feedforward[channel];
//    }
//    
//    void setDamping(SampleType newDamping)
//    {
//        mDamping = std::clamp(newDamping, 20.0, 20000.0);
//        mDampingFilter.setCutoffFrequency(mDamping);
//    }
//    
//    void setGain(SampleType newGain)
//    {
//        mGain = std::clamp(newGain, 0.0f, 1.0f);
//    }
//    
//private:
//    juce::dsp::DelayLine<SampleType> mDelayLine { 44100 };
//    juce::dsp::FirstOrderTPTFilter<SampleType> mDampingFilter;
//    
//    int mDelayInSamples { 4 };
//    
//    SampleType mDamping { 20000.0 };
//    SampleType mGain { 0.5 };
//    
//    std::vector<SampleType> drySample { };
//    std::vector<SampleType> delayOutput { };
//    std::vector<SampleType> feedforward { };
//    std::vector<SampleType> feedback { };
//    
//    SampleType mSampleRate { 44100.0 };
//};