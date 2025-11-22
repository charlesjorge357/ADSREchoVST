#include "DatorroHall.h"

//==============================================================================

DatorroHall::DatorroHall() {}
DatorroHall::~DatorroHall() {}

//==============================================================================

void DatorroHall::prepareAllpass(Allpass<float>& ap,
                                 const juce::dsp::ProcessSpec& spec,
                                 float delayMs,
                                 float gain)
{
    // delayMs is desired nominal delay; allocate a bit of headroom
    const int desiredSamples = (int) std::round((delayMs * 0.001f) * (float) spec.sampleRate);
    const int maxSamples     = juce::jmax(desiredSamples + 32, 4);

    ap.setMaximumDelayInSamples(maxSamples);
    ap.setDelay((float) desiredSamples);
    ap.setGain(gain);
    ap.prepare(spec);
    ap.reset();
}

//==============================================================================

void DatorroHall::prepare(const juce::dsp::ProcessSpec& spec)
{
    jassert(spec.numChannels >= 1);

    sampleRate = (int) spec.sampleRate;

    //-------------------------------------------
    // Tank damping filter (feedback high-cut)
    //-------------------------------------------
    loopDamping.prepare(spec);
    loopDamping.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    loopDamping.setCutoffFrequency(parameters.damping);
    loopDamping.reset();

    //-------------------------------------------
    // Prepare main tank delays
    //-------------------------------------------
    auto prepareDelay = [&](DelayLineWithSampleAccess<float>& d)
    {
        d.prepare(spec);
        d.reset();
    };

    prepareDelay(loopDelayL);
    prepareDelay(loopDelayR);

    


    // We’ll use ~80–90 ms base tank delays by default, scaled by roomSize later.
    const float defaultTankDelayL_ms = 80.0f;
    const float defaultTankDelayR_ms = 93.0f;

    const float defaultTankDelayL_samps =
        (defaultTankDelayL_ms * 0.001f) * (float) sampleRate;
    const float defaultTankDelayR_samps =
        (defaultTankDelayR_ms * 0.001f) * (float) sampleRate;

    maxDelaySamplesL  = (float) (loopDelayL.getNumSamples() - 2);
    maxDelaySamplesR  = (float) (loopDelayR.getNumSamples() - 2);

    baseDelaySamplesL = juce::jlimit(1.0f, maxDelaySamplesL, defaultTankDelayL_samps);
    baseDelaySamplesR = juce::jlimit(1.0f, maxDelaySamplesR, defaultTankDelayR_samps);

    currentDelayL_samps = baseDelaySamplesL;
    currentDelayR_samps = baseDelaySamplesR;

    //-------------------------------------------
    // Prepare early diffusion allpasses
    // (values are Dattorro-ish, but not strict)
    //-------------------------------------------
    // Early: relatively short allpasses, fairly strong feedback
    const float earlyAP1_ms = 12.0f;
    const float earlyAP2_ms = 20.0f;

    // Gains < 1.0, higher = more diffusion / more coloration
    const float earlyG1 = 0.75f;
    const float earlyG2 = 0.70f;

    prepareAllpass(earlyL1, spec, earlyAP1_ms, earlyG1);
    prepareAllpass(earlyL2, spec, earlyAP2_ms, earlyG2);
    prepareAllpass(earlyR1, spec, earlyAP1_ms * 1.1f, earlyG1);
    prepareAllpass(earlyR2, spec, earlyAP2_ms * 0.9f, earlyG2);

    //-------------------------------------------
    // Prepare late/tank diffusion allpasses
    //-------------------------------------------
    const float tankAP1_ms = 35.0f;
    const float tankAP2_ms = 60.0f;

    const float tankG1 = 0.72f;
    const float tankG2 = 0.70f;

    prepareAllpass(tankL1, spec, tankAP1_ms, tankG1);
    prepareAllpass(tankL2, spec, tankAP2_ms, tankG2);
    prepareAllpass(tankR1, spec, tankAP1_ms * 1.1f, tankG1);
    prepareAllpass(tankR2, spec, tankAP2_ms * 0.9f, tankG2);

    //-------------------------------------------
    // LFO for tank modulation
    //-------------------------------------------
    lfoParameters.waveform     = generatorWaveform::sin;
    lfoParameters.frequency_Hz = parameters.modRate;   // Hz
    lfoParameters.depth        = parameters.modDepth;  // 0..1
    lfo.setParameters(lfoParameters);
    lfo.prepare(spec);
    lfo.reset(spec.sampleRate);

    //-------------------------------------------
    // Channel buffers
    //-------------------------------------------
    channelInput.assign(2,  0.0f);
    channelFeedback.assign(2, 0.0f);
    channelOutput.assign(2, 0.0f);

    updateInternalParamsFromUserParams();
    reset();
}

//==============================================================================

void DatorroHall::reset()
{
    loopDamping.reset();

    auto resetAP = [&](Allpass<float>& ap) { ap.reset(); };
    resetAP(earlyL1);
    resetAP(earlyL2);
    resetAP(earlyR1);
    resetAP(earlyR2);
    resetAP(tankL1);
    resetAP(tankL2);
    resetAP(tankR1);
    resetAP(tankR2);

    loopDelayL.reset();
    loopDelayR.reset();

    std::fill(channelInput.begin(),    channelInput.end(),    0.0f);
    std::fill(channelFeedback.begin(), channelFeedback.end(), 0.0f);
    std::fill(channelOutput.begin(),   channelOutput.end(),   0.0f);

    lfo.reset(sampleRate);
}

//==============================================================================

void DatorroHall::updateInternalParamsFromUserParams()
{
    // Clamp and derive things that are used in DSP
    parameters.roomSize = juce::jlimit(0.25f, 1.75f, parameters.roomSize);
    parameters.decayTime = juce::jlimit(0.0f, 0.99f, parameters.decayTime);
    parameters.mix = juce::jlimit(0.0f, 1.0f, parameters.mix);

    // Update damping filter cut-off
    loopDamping.setCutoffFrequency(parameters.damping);

    // Update LFO params
    lfoParameters.frequency_Hz = parameters.modRate;
    lfoParameters.depth        = parameters.modDepth;
    lfo.setParameters(lfoParameters);
}

//==============================================================================

void DatorroHall::processBlock(juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    auto* left  = buffer.getWritePointer(0);
    auto* right = (numChannels > 1 ? buffer.getWritePointer(1) : nullptr);

    // Snap/derive parameters once per block
    const float mix      = parameters.mix;
    const float dryMix   = 1.0f - mix;
    const float decay    = parameters.decayTime;
    const float roomSize = parameters.roomSize;
    const float modDepth = juce::jlimit(0.0f, 1.0f, parameters.modDepth);

    // Scale base delays by room size
    const float baseL = juce::jlimit(1.0f, maxDelaySamplesL,
                                     baseDelaySamplesL * roomSize);
    const float baseR = juce::jlimit(1.0f, maxDelaySamplesR,
                                     baseDelaySamplesR * roomSize);

 

    // Stereo cross-feedback amount
    const float crossFeed = 0.30f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float inL = left[n];
        const float inR = (right != nullptr ? right[n] : inL);

        channelInput[0] = inL;
        channelInput[1] = inR;

        //--------------------------------------------------------------
        // Early diffusion per channel (two allpasses L, two allpasses R)
        //--------------------------------------------------------------
        // LEFT
        earlyL1.pushSample(0, channelInput[0]);
        float earlyL = earlyL1.popSample(0);

        earlyL2.pushSample(0, earlyL);
        earlyL = earlyL2.popSample(0);

        // RIGHT
        earlyR1.pushSample(0, channelInput[1]);
        float earlyR = earlyR1.popSample(0);

        earlyR2.pushSample(0, earlyR);
        earlyR = earlyR2.popSample(0);

        //--------------------------------------------------------------
        // LFO modulation (one per sample, used for both channels)
        //--------------------------------------------------------------
        lfoOutput = lfo.renderAudioOutput();
        const float lfoL = (float) lfoOutput.normalOutput;          // left
        const float lfoR = (float) lfoOutput.quadPhaseOutput_pos;   // right

        // Much gentler modulation (0.5% instead of 3%)
        const float modRatio = 0.005f;

        const float maxModL_samps = baseL * modRatio * modDepth;
        const float maxModR_samps = baseR * modRatio * modDepth;

        // raw target delay from LFO
        float targetDelayL_samps = baseL + maxModL_samps * lfoL;
        float targetDelayR_samps = baseR + maxModR_samps * lfoR;

        // clamp
        targetDelayL_samps = juce::jlimit(1.0f, maxDelaySamplesL, targetDelayL_samps);
        targetDelayR_samps = juce::jlimit(1.0f, maxDelaySamplesR, targetDelayR_samps);

        // slew-smoothed delay to avoid zippering
        const float slew = 0.001f; // very gentle smoothing
        currentDelayL_samps += slew * (targetDelayL_samps - currentDelayL_samps);
        currentDelayR_samps += slew * (targetDelayR_samps - currentDelayR_samps);

        const float delayL_samps = currentDelayL_samps;
        const float delayR_samps = currentDelayR_samps;


        //--------------------------------------------------------------
        // Push early-diffused + feedback into tank delay lines
        //--------------------------------------------------------------
        const float tankInL = 0.5f * (earlyL + channelFeedback[0]);
        const float tankInR = 0.5f * (earlyR + channelFeedback[1]);


        loopDelayL.pushSample(0, tankInL);
        loopDelayR.pushSample(0, tankInR);

        // Fractional read with your custom delay line
        float tankOutL = loopDelayL.readFractional(0, delayL_samps);
        float tankOutR = loopDelayR.readFractional(0, delayR_samps);

        //--------------------------------------------------------------
        // Late diffusion: two allpasses in series per channel
        //--------------------------------------------------------------
        // LEFT
        tankL1.pushSample(0, tankOutL);
        float diffL = tankL1.popSample(0);

        tankL2.pushSample(0, diffL);
        diffL = tankL2.popSample(0);

        // RIGHT
        tankR1.pushSample(0, tankOutR);
        float diffR = tankR1.popSample(0);

        tankR2.pushSample(0, diffR);
        diffR = tankR2.popSample(0);

        //--------------------------------------------------------------
        // Damping and feedback update (with stereo cross-feed)
        //--------------------------------------------------------------
        float dampedL = loopDamping.processSample(0, diffL);
        float dampedR = loopDamping.processSample(1, diffR);


        const float fbL = (dampedL + crossFeed * dampedR) * decay;
        const float fbR = (dampedR + crossFeed * dampedL) * decay;

        channelFeedback[0] = fbL;
        channelFeedback[1] = fbR;

        channelOutput[0] = dampedL;
        channelOutput[1] = dampedR;

        //--------------------------------------------------------------
        // Mix dry/wet and write back
        //--------------------------------------------------------------
        left[n] = dryMix * inL + mix * channelOutput[0];

        if (right != nullptr)
            right[n] = dryMix * inR + mix * channelOutput[1];
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
    updateInternalParamsFromUserParams();
}
