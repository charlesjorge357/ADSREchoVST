#include "DatorroHall.h"
#include <algorithm>
#include <cmath>

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

    //=====================================
    // Reset channels / feedback
    //=====================================
    channelInput.assign(2, 0.0f);
    channelOutput.assign(2, 0.0f);

    std::fill(std::begin(feedbackL), std::end(feedbackL), 0.0f);
    std::fill(std::begin(feedbackR), std::end(feedbackR), 0.0f);

    //=====================================
    // Loop Damping Filter
    //=====================================
    loopDamping.prepare(spec);
    loopDamping.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    loopDamping.setCutoffFrequency(parameters.damping);
    loopDamping.reset();

    // Pre Delay
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);
    preDelayL.reset();
    preDelayR.reset();

    erL.prepare(spec);
    erR.prepare(spec);
    erL.reset();
    erR.reset();



    //=====================================
    // Prepare delay lines (4 per channel)
    //=====================================
    auto prepDL = [&](DelayLineWithSampleAccess<float>& d)
    {
        d.prepare(spec);
        d.reset();
    };

    prepDL(tankDelayL1);
    prepDL(tankDelayL2);
    prepDL(tankDelayL3);
    prepDL(tankDelayL4);

    prepDL(tankDelayR1);
    prepDL(tankDelayR2);
    prepDL(tankDelayR3);
    prepDL(tankDelayR4);

    for (int i = 0; i < 4; ++i)
    {
        dampingFiltersL[i].prepare(spec);
        dampingFiltersL[i].setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
        dampingFiltersL[i].reset();

        dampingFiltersR[i].prepare(spec);
        dampingFiltersR[i].setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
        dampingFiltersR[i].reset();
    }

    for (int i = 0; i < 4; ++i)
    {
        extraDampingL[i].prepare(sampleRate, 0.25f);
        extraDampingR[i].prepare(sampleRate, 0.25f);
    }



    //=====================================
    // Bright-hall base delay times (ms)
    // Valhalla-ish spacing
    //=====================================
    constexpr float baseMs[4] = {
        130.0f,   // line 1
        155.0f,   // line 2
        177.0f,   // line 3
        199.0f    // line 4
    };

    // Convert to samples & clamp
    DelayLineWithSampleAccess<float>* linesL[4] =
    { &tankDelayL1, &tankDelayL2, &tankDelayL3, &tankDelayL4 };

    DelayLineWithSampleAccess<float>* linesR[4] =
    { &tankDelayR1, &tankDelayR2, &tankDelayR3, &tankDelayR4 };

    for (int i = 0; i < 4; ++i)
    {
        const float baseSamps = baseMs[i] * 0.001f * sampleRate;

        maxDelaySamplesL[i] = (float) (linesL[i]->getNumSamples() - 2);
        maxDelaySamplesR[i] = (float) (linesR[i]->getNumSamples() - 2);

        baseDelaySamplesL[i] = juce::jlimit(1.0f, maxDelaySamplesL[i], baseSamps);
        baseDelaySamplesR[i] = juce::jlimit(1.0f, maxDelaySamplesR[i], baseSamps);

        currentDelayL_samps[i] = baseDelaySamplesL[i];
        currentDelayR_samps[i] = baseDelaySamplesR[i];
    }

    //=====================================
    // Prepare early diffusion allpasses
    //=====================================
    auto prepAP = [&](Allpass<float>& ap, float delayMs, float gain)
    {
        prepareAllpass(ap, spec, delayMs, gain);
    };

    for (int i = 0; i < ER_count; ++i)
    {
        ER_tapSamplesLeft[i]  = ER_tapTimesMsLeft[i]  * 0.001f * sampleRate;
        ER_tapSamplesRight[i] = ER_tapTimesMsRight[i] * 0.001f * sampleRate;
    }


    // Early diffusion (4 APs), strong diffusion
    prepAP(earlyL1,  8.0f, 0.70f);
    prepAP(earlyL2, 12.0f, 0.72f);
    prepAP(earlyL3, 15.0f, 0.68f);
    prepAP(earlyL4, 22.0f, 0.70f);

    prepAP(earlyR1,  8.8f, 0.70f);
    prepAP(earlyR2, 10.5f, 0.72f);
    prepAP(earlyR3, 16.0f, 0.68f);
    prepAP(earlyR4, 21.0f, 0.70f);

    //=====================================
    // Prepare tank diffusion APs (per-line)
    //=====================================
    prepAP(tankLAP1, 35.0f, 0.72f);
    prepAP(tankLAP2, 55.0f, 0.70f);
    prepAP(tankLAP3, 78.0f, 0.72f);
    prepAP(tankLAP4, 92.0f, 0.70f);

    prepAP(tankRAP1, 35.0f, 0.72f);
    prepAP(tankRAP2, 55.0f, 0.70f);
    prepAP(tankRAP3, 78.0f, 0.72f);
    prepAP(tankRAP4, 92.0f, 0.70f);

    //=====================================
    // LFO Setup
    //=====================================
    lfoParameters.waveform     = generatorWaveform::sin;
    lfoParameters.frequency_Hz = parameters.modRate;   // user param
    lfoParameters.depth        = parameters.modDepth;

    lfo.setParameters(lfoParameters);
    lfo.prepare(spec);
    lfo.reset(sampleRate);

    //=====================================
    // Estimate loop time for RT60 mapping
    // (sum of delays + AP times)
    //=====================================
    float totalDelaySamps = 0.0f;
    for (int i = 0; i < 4; ++i)
        totalDelaySamps += baseDelaySamplesL[i];

    // Convert early+late AP delays to seconds
    const float apDelayMs =
          8.0f  + 12.0f + 15.0f + 22.0f   // early
        + 35.0f + 55.0f + 78.0f + 92.0f;  // tank

    estimatedLoopTimeSeconds =
        (totalDelaySamps / sampleRate) +
        (apDelayMs * 0.001f);

    // Clamp / update internal params (no RT60 remap here)
    updateInternalParamsFromUserParams();

    //=====================================
    // Done
    //=====================================
    reset();
}

//==============================================================================

void DatorroHall::reset()
{
    loopDamping.reset();

    auto resetAP = [&](Allpass<float>& ap) { ap.reset(); };

    // Early APs
    resetAP(earlyL1);
    resetAP(earlyL2);
    resetAP(earlyL3);
    resetAP(earlyL4);

    resetAP(earlyR1);
    resetAP(earlyR2);
    resetAP(earlyR3);
    resetAP(earlyR4);

    // Tank APs
    resetAP(tankLAP1);
    resetAP(tankLAP2);
    resetAP(tankLAP3);
    resetAP(tankLAP4);

    resetAP(tankRAP1);
    resetAP(tankRAP2);
    resetAP(tankRAP3);
    resetAP(tankRAP4);

    // Tank delay lines
    tankDelayL1.reset();
    tankDelayL2.reset();
    tankDelayL3.reset();
    tankDelayL4.reset();

    tankDelayR1.reset();
    tankDelayR2.reset();
    tankDelayR3.reset();
    tankDelayR4.reset();

    // Feedback & buffers
    std::fill(std::begin(feedbackL), std::end(feedbackL), 0.0f);
    std::fill(std::begin(feedbackR), std::end(feedbackR), 0.0f);

    std::fill(channelInput.begin(),  channelInput.end(),  0.0f);
    std::fill(channelOutput.begin(), channelOutput.end(), 0.0f);

    // Smoothed delay times
    for (int i = 0; i < 4; ++i)
    {
        currentDelayL_samps[i] = baseDelaySamplesL[i];
        currentDelayR_samps[i] = baseDelaySamplesR[i];
    }

    lfo.reset(sampleRate);
}

//==============================================================================

void DatorroHall::updateInternalParamsFromUserParams()
{
    parameters.roomSize = juce::jlimit(0.25f, 1.75f, parameters.roomSize);

    // keep decay in SECONDS
    parameters.decayTime = juce::jlimit(0.1f, 20.0f, parameters.decayTime);

    parameters.mix = juce::jlimit(0.0f, 1.0f, parameters.mix);

    loopDamping.setCutoffFrequency(parameters.damping);

    for (int i = 0; i < 4; ++i)
    {
        dampingFiltersL[i].setCutoffFrequency(parameters.damping);
        dampingFiltersR[i].setCutoffFrequency(parameters.damping);
    }


    lfoParameters.frequency_Hz = parameters.modRate;
    lfoParameters.depth        = parameters.modDepth;
    lfo.setParameters(lfoParameters);

    float pdMs = parameters.preDelay;     // new param (ms)
    pdMs = juce::jlimit(0.0f, 200.0f, pdMs);

    preDelaySamples = pdMs * 0.001f * sampleRate;

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

    //===============================
    // Snap parameters
    //===============================
    const float mix      = parameters.mix;
    const float dryMix   = 1.0f - mix;
    const float decaySec = juce::jlimit(0.1f, 20.0f, parameters.decayTime);
    const float roomSize = juce::jlimit(0.25f, 1.75f, parameters.roomSize);
    const float modDepth = parameters.modDepth;

    //===============================
    // Compute RT60 feedback gain
    // Using e^(-3T_loop / RT60)
    //===============================
    const float fb = std::exp(-3.0f * estimatedLoopTimeSeconds / decaySec);


    const float feedbackGain = juce::jlimit(0.0f, 0.9999f, fb);

    //===============================
    // Stereo crossfeed (light)
    //===============================
    const float stereoCross = 0.15f;

    const float slew = 0.001f; // Smooth modulation

    //===============================
    // Process samples
    //===============================
    for (int n = 0; n < numSamples; ++n)
    {
        // --- TRUE DRY signal (captured before any pre-delay!) ---
        const float dryL = left[n];
        const float dryR = (right ? right[n] : dryL);

        //=========================================================
        // PRE-DELAY (WET PATH ONLY)
        //=========================================================
        preDelayL.pushSample(0, dryL);
        preDelayR.pushSample(0, dryR);

        float inL = preDelayL.readFractional(0, preDelaySamples);
        float inR = preDelayR.readFractional(0, preDelaySamples);

        channelInput[0] = inL;
        channelInput[1] = inR;

        erL.pushSample(0, dryL);
        erR.pushSample(0, dryR);

        float erOutL = 0.0f;
        float erOutR = 0.0f;

        for (int i = 0; i < 6; ++i)
        {
            erOutL += ER_gains[i] * erL.readFractional(0, ER_tapSamplesLeft[i]);
            erOutR += ER_gains[i] * erR.readFractional(0, ER_tapSamplesRight[i]);
        }



        //===========================
        // EARLY DIFFUSION (4 APs)
        //===========================

        // Replace dry → diffusion input with early reflections
        float eL = erOutL;
        earlyL1.pushSample(0, eL);
        eL = earlyL1.popSample(0);
        earlyL2.pushSample(0, eL);
        eL = earlyL2.popSample(0);
        earlyL3.pushSample(0, eL);
        eL = earlyL3.popSample(0);
        earlyL4.pushSample(0, eL);
        eL = earlyL4.popSample(0);

        float eR = erOutR;
        earlyR1.pushSample(0, eR);
        eR = earlyR1.popSample(0);
        earlyR2.pushSample(0, eR);
        eR = earlyR2.popSample(0);
        earlyR3.pushSample(0, eR);
        eR = earlyR3.popSample(0);
        earlyR4.pushSample(0, eR);
        eR = earlyR4.popSample(0);


        //===========================
        // LFO per-sample
        //===========================
        lfoOutput = lfo.renderAudioOutput();
            // LFO gives us only 2 useful outputs: normal + quad
        const float lfo0 = (float) lfoOutput.normalOutput;
        const float lfo90 = (float) lfoOutput.quadPhaseOutput_pos;

        // Synthesize 4 decorrelated modulation values
        // (simple nonlinear warping—cheap but effective)
        const float lfoVals[4] =
        {
            lfo0,
            lfo90,
            std::tanh(lfo0 + 0.5f * lfo90),
            std::tanh(lfo90 - 0.5f * lfo0)
        };




        //===========================
        // PER-LINE MODULATION (with decay-dependent density scaling)
        //===========================

        // Decay → echo-density scaling factor
        const float normDecay = juce::jlimit(0.0f, 1.0f, decaySec / 20.0f);
        const float densityScale = 1.0f + 0.20f * normDecay;  // up to +20% delay stretch

        for (int i = 0; i < 4; ++i)
        {
            // Base delay now stretches with decay length
            const float baseL = juce::jlimit(1.0f, maxDelaySamplesL[i],
                                             baseDelaySamplesL[i] * roomSize * densityScale);

            const float baseR = juce::jlimit(1.0f, maxDelaySamplesR[i],
                                             baseDelaySamplesR[i] * roomSize * densityScale);

            const float modRatio  = 0.01f;  // 1% modulation
            const float modSampsL = baseL * modRatio * modDepth * lfoVals[i];
            const float modSampsR = baseR * modRatio * modDepth * lfoVals[i];

            const float targetL = juce::jlimit(1.0f, maxDelaySamplesL[i],
                                               baseL + modSampsL);
            const float targetR = juce::jlimit(1.0f, maxDelaySamplesR[i],
                                               baseR + modSampsR);

            currentDelayL_samps[i] += slew * (targetL - currentDelayL_samps[i]);
            currentDelayR_samps[i] += slew * (targetR - currentDelayR_samps[i]);
        }



        //===========================
        // PUSH INPUT + FEEDBACK
        //===========================
        float tankInputL[4];
        float tankInputR[4];

        // Merge early-diffused input with feedback
        for (int i = 0; i < 4; ++i)
        {
            // 0.5 to keep internal gain under control
            tankInputL[i] = 0.8f * (eL + feedbackL[i]);
            tankInputR[i] = 0.8f * (eR + feedbackR[i]);
        }

        // Push to tank delay lines
        tankDelayL1.pushSample(0, tankInputL[0]);
        tankDelayL2.pushSample(0, tankInputL[1]);
        tankDelayL3.pushSample(0, tankInputL[2]);
        tankDelayL4.pushSample(0, tankInputL[3]);

        tankDelayR1.pushSample(0, tankInputR[0]);
        tankDelayR2.pushSample(0, tankInputR[1]);
        tankDelayR3.pushSample(0, tankInputR[2]);
        tankDelayR4.pushSample(0, tankInputR[3]);

        //===========================
        // READ TANK OUTPUTS
        //===========================
        float rawL[4] = {
            tankDelayL1.readFractional(0, currentDelayL_samps[0]),
            tankDelayL2.readFractional(0, currentDelayL_samps[1]),
            tankDelayL3.readFractional(0, currentDelayL_samps[2]),
            tankDelayL4.readFractional(0, currentDelayL_samps[3])
        };

        float rawR[4] = {
            tankDelayR1.readFractional(0, currentDelayR_samps[0]),
            tankDelayR2.readFractional(0, currentDelayR_samps[1]),
            tankDelayR3.readFractional(0, currentDelayR_samps[2]),
            tankDelayR4.readFractional(0, currentDelayR_samps[3])
        };

        //=====================================
        // PER-LINE DAMPING (Valhalla-style HF shaping)
        //=====================================
        for (int i = 0; i < 4; ++i)
        {
            rawL[i] = extraDampingL[i].process(rawL[i]);
            rawR[i] = extraDampingR[i].process(rawR[i]);

        }



        //===========================
        // TANK INTERNAL DIFFUSION
        // one AP per line
        //===========================
        float diffL[4] = { rawL[0], rawL[1], rawL[2], rawL[3] };
        float diffR[4] = { rawR[0], rawR[1], rawR[2], rawR[3] };

        tankLAP1.pushSample(0, diffL[0]); diffL[0] = tankLAP1.popSample(0);
        tankLAP2.pushSample(0, diffL[1]); diffL[1] = tankLAP2.popSample(0);
        tankLAP3.pushSample(0, diffL[2]); diffL[2] = tankLAP3.popSample(0);
        tankLAP4.pushSample(0, diffL[3]); diffL[3] = tankLAP4.popSample(0);

        tankRAP1.pushSample(0, diffR[0]); diffR[0] = tankRAP1.popSample(0);
        tankRAP2.pushSample(0, diffR[1]); diffR[1] = tankRAP2.popSample(0);
        tankRAP3.pushSample(0, diffR[2]); diffR[2] = tankRAP3.popSample(0);
        tankRAP4.pushSample(0, diffR[3]); diffR[3] = tankRAP4.popSample(0);

        //===========================
        // APPLY FDN SCATTERING (Householder)
        //===========================
        float scatterL[4];
        float scatterR[4];

        applyFDNScattering(diffL, scatterL);
        applyFDNScattering(diffR, scatterR);

        //===========================
        // DAMPING + FEEDBACK UPDATE WITH STEREO CROSSFEED
        //===========================
        for (int i = 0; i < 4; ++i)
        {
            // scatterL/R are already damped & scattered
            const float sL = scatterL[i];
            const float sR = scatterR[i];

            // Stereo crossfeed
            const float dL = sL + stereoCross * sR;
            const float dR = sR + stereoCross * sL;

            // Apply loop damping (lowpass)
            float dampedL = dampingFiltersL[i].processSample(0, dL);
            float dampedR = dampingFiltersR[i].processSample(0, dR);



            // Then apply feedback gain
            feedbackL[i] = dampedL * feedbackGain;
            feedbackR[i] = dampedR * feedbackGain;

        }

        //===========================
        // OUTPUT MIX (use scattered signal for richness)
        //===========================
        float outL = 0.35f * (scatterL[0] + scatterL[2])
           + 0.25f * (scatterL[1] + scatterL[3]);

        float outR = 0.35f * (scatterR[0] + scatterR[2])
           + 0.25f * (scatterR[1] + scatterR[3]);


        channelOutput[0] = outL;
        channelOutput[1] = outR;

        left[n] = dryMix * dryL + mix * outL;

        if (right)
            right[n] = dryMix * dryR + mix * outR;
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

//==============================================================================

void DatorroHall::applyFDNScattering(const float in[4], float (&out)[4]) const
{
    // A 4x4 Householder matrix:
    // H = I - (2 / N) * 11^T   → for N = 4 → I - 0.5 * 11^T
    //
    // Output line i:
    // out[i] = in[i] - 0.5 * (in[0] + in[1] + in[2] + in[3])

    const float sum =
        in[0] +
        in[1] +
        in[2] +
        in[3];

    const float scaled = 0.5f * sum;

    out[0] = in[0] - scaled;
    out[1] = in[1] - scaled;
    out[2] = in[2] - scaled;
    out[3] = in[3] - scaled;
}
