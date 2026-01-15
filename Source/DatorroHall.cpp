// ================================================================
// DatorroHall.cpp - PATCHED to use MLDatorroParameters
// 

#include "DatorroHall.h"

//==============================================================================
DatorroHall::DatorroHall() {}

DatorroHall::~DatorroHall() {}

//==============================================================================
void DatorroHall::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = static_cast<int>(spec.sampleRate);

    // Prepare filters
    loopDamping.prepare(spec);
    loopDamping.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    loopDamping.setCutoffFrequency(parameters.damping * mlParameters.dampingScale);
    loopDamping.reset();

    // Prepare all DelayLineWithSampleAccess delay lines
    auto prepareCustomDelay = [&](auto& d) { 
        d.prepare(spec); 
        d.reset(); 
    };

    prepareCustomDelay(loopDelayL1);
    prepareCustomDelay(loopDelayL2);
    prepareCustomDelay(loopDelayL3);
    prepareCustomDelay(loopDelayL4);
    prepareCustomDelay(loopDelayR1);
    prepareCustomDelay(loopDelayR2);
    prepareCustomDelay(loopDelayR3);
    prepareCustomDelay(loopDelayR4);

    // Prepare standard JUCE delay lines for allpass and filters
    auto prepareJuceDelay = [&](auto& d) {
        d.prepare(spec);
        d.reset();
    };

    prepareJuceDelay(inputBandwidth);
    prepareJuceDelay(feedbackDamping);
    prepareJuceDelay(inputZ);

    prepareJuceDelay(allpassL1);
    prepareJuceDelay(allpassL2);
    prepareJuceDelay(allpassL3Inner);
    prepareJuceDelay(allpassL3Outer);
    prepareJuceDelay(allpassL4Innermost);
    prepareJuceDelay(allpassL4Inner);
    prepareJuceDelay(allpassL4Outer);

    prepareJuceDelay(allpassR1);
    prepareJuceDelay(allpassR2);
    prepareJuceDelay(allpassR3Inner);
    prepareJuceDelay(allpassR3Outer);
    prepareJuceDelay(allpassR4Innermost);
    prepareJuceDelay(allpassR4Inner);
    prepareJuceDelay(allpassR4Outer);

    prepareJuceDelay(allpassChorusL);
    prepareJuceDelay(allpassChorusR);

    // Prepare LFO
    lfoParameters.frequency_Hz = 0.5;
    lfoParameters.depth = 1.0;
    lfoParameters.waveform = generatorWaveform::sin;
    lfo.setParameters(lfoParameters);
    lfo.prepare(spec);
    lfo.reset(spec.sampleRate);

    // Resize channel vectors
    channelInput.assign(2, 0.0f);
    channelFeedback.assign(2, 0.0f);
    channelOutput.assign(2, 0.0f);
}

//==============================================================================
void DatorroHall::reset()
{
    loopDamping.reset();

    auto resetDelay = [&](auto& d) { d.reset(); };

    resetDelay(inputBandwidth);
    resetDelay(feedbackDamping);
    resetDelay(inputZ);

    resetDelay(loopDelayL1);
    resetDelay(loopDelayL2);
    resetDelay(loopDelayL3);
    resetDelay(loopDelayL4);
    resetDelay(loopDelayR1);
    resetDelay(loopDelayR2);
    resetDelay(loopDelayR3);
    resetDelay(loopDelayR4);

    resetDelay(allpassL1);
    resetDelay(allpassL2);
    resetDelay(allpassL3Inner);
    resetDelay(allpassL3Outer);
    resetDelay(allpassL4Innermost);
    resetDelay(allpassL4Inner);
    resetDelay(allpassL4Outer);

    resetDelay(allpassR1);
    resetDelay(allpassR2);
    resetDelay(allpassR3Inner);
    resetDelay(allpassR3Outer);
    resetDelay(allpassR4Innermost);
    resetDelay(allpassR4Inner);
    resetDelay(allpassR4Outer);

    resetDelay(allpassChorusL);
    resetDelay(allpassChorusR);

    lfo.reset(sampleRate);
}

//==============================================================================
void DatorroHall::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // ========================================
    // GET ML PARAMETERS (NEW!)
    // ========================================
    const float diffCoeff1 = mlParameters.diffusionCoeff1;
    const float diffCoeff2 = mlParameters.diffusionCoeff2;
    const float decayScale = mlParameters.decayScale;
    const float dampingScale = mlParameters.dampingScale;
    
    // Early allpass scales
    const float earlyScale0 = mlParameters.earlyAllpassScales[0];
    const float earlyScale1 = mlParameters.earlyAllpassScales[1];
    
    // Loop delay scales (we'll use index 1 for the main modulated delay)
    const float loopScaleL = mlParameters.loopDelayScalesL[1];
    const float loopScaleR = mlParameters.loopDelayScalesR[1];

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        
        // Select the appropriate loop scale for this channel
        const float loopScale = (channel == 0) ? loopScaleL : loopScaleR;
        const float earlyScale = (channel == 0) ? earlyScale0 : earlyScale1;

        for (int n = 0; n < numSamples; ++n)
        {
            const float input = data[n];

            // Input bandwidth filter (simple delay as lowpass)
            inputBandwidth.pushSample(0, input);
            const float inputFiltered = inputBandwidth.popSample(0);

            // ========================================
            // Early diffusion - Allpass 1
            // NOW USING: diffCoeff1 instead of hardcoded 0.75
            // ========================================
            auto& ap1Delay = (channel == 0) ? allpassL1 : allpassR1;
            float ap1Out = ap1Delay.popSample(0);
            float feedback1 = ap1Out * diffCoeff1 * parameters.diffusion;  // CHANGED!
            ap1Delay.pushSample(0, inputFiltered + feedback1);
            float ap1Result = ap1Out - feedback1;

            // ========================================
            // Early diffusion - Allpass 2
            // NOW USING: diffCoeff2 instead of hardcoded 0.625
            // ========================================
            auto& ap2Delay = (channel == 0) ? allpassL2 : allpassR2;
            float ap2Out = ap2Delay.popSample(0);
            float feedback2 = ap2Out * diffCoeff2 * parameters.diffusion;  // CHANGED!
            ap2Delay.pushSample(0, ap1Result + feedback2);
            float ap2Result = ap2Out - feedback2;

            // LFO modulation
            SignalGenData lfoData = lfo.renderAudioOutput();
            const float lfoValue = static_cast<float>(lfoData.normalOutput);
            
            // ========================================
            // Calculate modulated delay time
            // NOW USING: loopScale from ML parameters
            // ========================================
            const float modulatedDelay = baseLoopDelay * loopScale *  // CHANGED!
                                         (1.0f + parameters.modDepth * lfoValue) * 
                                         parameters.roomSize;

            // Main delay network using DelayLineWithSampleAccess
            auto& loopDelay = (channel == 0) ? loopDelayL2 : loopDelayR2;
            
            // Set the delay length
            loopDelay.setDelay(static_cast<int>(std::max(1.0f, modulatedDelay)));
            
            // Read from delay line
            float delayOutput = loopDelay.popSample(0);
            
            // Write new input to delay line
            loopDelay.pushSample(0, ap2Result + channelFeedback[channel]);

            // Late diffusion & damping
            float damped = loopDamping.processSample(0, delayOutput);
            
            // ========================================
            // Feedback with decay
            // NOW USING: decayScale from ML parameters
            // ========================================
            channelFeedback[channel] = damped * parameters.decayTime * decayScale;  // CHANGED!

            // Output
            channelOutput[channel] = damped;
            data[n] = (input * (1.0f - parameters.mix)) + (damped * parameters.mix);
        }
    }
}

//==============================================================================
ReverbProcessorParameters& DatorroHall::getParameters()
{
    return parameters;
}

//==============================================================================
void DatorroHall::setParameters(const ReverbProcessorParameters& params)
{
    parameters = params;

    parameters.roomSize = juce::jlimit(0.25f, 1.75f, parameters.roomSize);

    // ========================================
    // Update damping filter
    // NOW USING: dampingScale from ML parameters
    // ========================================
    loopDamping.setCutoffFrequency(parameters.damping * mlParameters.dampingScale);  // CHANGED!

    // Update LFO
    lfoParameters.frequency_Hz = parameters.modRate;
    lfoParameters.depth = parameters.modDepth;
    lfo.setParameters(lfoParameters);
}
