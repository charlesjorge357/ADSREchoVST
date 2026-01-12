// HybridPlate.cpp
#include "HybridPlate.h"
#include <algorithm>
#include <cmath>

HybridPlate::HybridPlate() = default;
HybridPlate::~HybridPlate() = default;

//==============================================================================

void HybridPlate::prepareAllpass(Allpass<float>& ap,
                                 const juce::dsp::ProcessSpec& spec,
                                 float delayMs,
                                 float gain)
{
    const int desiredSamples =
        (int) std::round((delayMs * 0.001f) * (float) spec.sampleRate);
    const int maxSamples = juce::jmax(desiredSamples + 32, 4);

    ap.setMaximumDelayInSamples(maxSamples);
    ap.setDelay((float) desiredSamples);
    ap.setGain(gain);
    ap.prepare(spec);
    ap.reset();
}

//==============================================================================

void HybridPlate::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = (int) spec.sampleRate;

    // -------------------------
    // Pre-delay setup
    // -------------------------
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);
    preDelayL.reset();
    preDelayR.reset();

    // -------------------------
    // Early diffusion (plate-style, shorter than hall)
    // -------------------------
    const float earlyDelaysMs[4] = { 3.1f, 7.7f, 11.3f, 15.9f };
    const float earlyGain        = 0.82f;   // diffusive but not ringy

    for (int i = 0; i < 4; ++i)
    {
        const float dL = earlyDelaysMs[i];
        const float dR = earlyDelaysMs[i] * 1.11f; // slight L/R decorrelation

        prepareAllpass(earlyL[i], spec, dL, earlyGain);
        prepareAllpass(earlyR[i], spec, dR, earlyGain);
    }

    // -------------------------
    // FDN delay lines
    // -------------------------
    const float fdnDelayMs[fdnCount] = {
        31.3f,
        37.1f,
        43.7f,
        55.9f
    };


    for (int i = 0; i < fdnCount; ++i)
    {
        fdnLines[i].prepare(spec);
        fdnLines[i].reset();

        const float baseSamps = fdnDelayMs[i] * 0.001f * (float) sampleRate;
        maxDelaySamples[i]    = (float) (fdnLines[i].getNumSamples() - 2);

        baseDelaySamples[i]    = juce::jlimit(1.0f, maxDelaySamples[i], baseSamps);
        currentDelaySamples[i] = baseDelaySamples[i];
    }

    // -------------------------
    // Damping filters per FDN line
    // -------------------------
    for (int i = 0; i < fdnCount; ++i)
    {
        dampingFilters[i].prepare(spec);
        dampingFilters[i].reset();
        dampingFilters[i].setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
    }

    for (int i = 0; i < fdnCount; ++i)
        extraDampL[i].prepare(sampleRate, 0.25f); // stable default


    // high shelf for no ring
    for (int i = 0; i < fdnCount; ++i)
    {
        juce::dsp::IIR::Coefficients<float>::Ptr coeff =
            juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate,
                10000.0f,   // frequency where ringing builds
                0.707f,     // Q
                0.95f        // gain factor < 1.0 removes ringing
            );

        highShelfFilters[i].prepare(spec);

        *highShelfFilters[i].coefficients = *coeff;
        
        highShelfFilters[i].reset();
    }

    // High pass for no womp
    for (int i = 0; i < fdnCount; ++i)
        hpFDN[i].prepare(sampleRate, 60.0f);  // try 60 Hz



    // -------------------------
    // LFO setup (for FDN modulation)
    // -------------------------
    lfoParameters.waveform     = generatorWaveform::sin;
    lfoParameters.frequency_Hz = parameters.modRate;
    lfoParameters.depth        = parameters.modDepth;

    lfo.setParameters(lfoParameters);
    lfo.prepare(spec);
    lfo.reset(sampleRate);

    // -------------------------
    // Estimate loop time for RT60 mapping
    // (Sum of FDN delays + early diffuser times)
    // -------------------------
    float meanFDNDelaySamps = 0.0f;
    for (int i = 0; i < fdnCount; ++i)
        meanFDNDelaySamps += baseDelaySamples[i];
    meanFDNDelaySamps /= fdnCount;

    // FDN recirculation loop (~ one average pass)
    estimatedLoopTimeSeconds = (meanFDNDelaySamps / sampleRate);


    // -------------------------
    // Internal buffers
    // -------------------------
    channelInput.assign(2, 0.0f);
    channelOutput.assign(2, 0.0f);

    updateInternalParamsFromUserParams();
    reset();
}

//==============================================================================

void HybridPlate::reset()
{
    preDelayL.reset();
    preDelayR.reset();

    for (int i = 0; i < 4; ++i)
    {
        earlyL[i].reset();
        earlyR[i].reset();
    }

    for (int i = 0; i < fdnCount; ++i)
    {
        fdnLines[i].reset();
        dampingFilters[i].reset();
        currentDelaySamples[i] = baseDelaySamples[i];
    }

    std::fill(channelInput.begin(),  channelInput.end(),  0.0f);
    std::fill(channelOutput.begin(), channelOutput.end(), 0.0f);

    lfo.reset(sampleRate);
}

//==============================================================================

void HybridPlate::updateInternalParamsFromUserParams()
{
    parameters.roomSize  = juce::jlimit(0.25f, 1.75f, parameters.roomSize);
    parameters.decayTime = juce::jlimit(0.1f, 20.0f,  parameters.decayTime);
    parameters.mix       = juce::jlimit(0.0f, 1.0f,   parameters.mix);

    // Pre-delay in ms -> samples
    float pdMs = juce::jlimit(0.0f, 200.0f, parameters.preDelay);
    preDelaySamples = pdMs * 0.001f * (float) sampleRate;

    // Treat damping as a cutoff frequency in Hz, like DatorroHall
    float cutoffHz = juce::jlimit(500.0f, 20000.0f, parameters.damping);

    for (int i = 0; i < fdnCount; ++i)
        dampingFilters[i].setCutoffFrequency(cutoffHz);

    // Derive a 0..1 "darkness" from the cutoff
    float darkness = 1.0f - (cutoffHz - 500.0f) / (20000.0f - 500.0f);
    darkness = juce::jlimit(0.0f, 1.0f, darkness);

    // Psycho damping: modest amount, more when darker
    for (int i = 0; i < fdnCount; ++i)
    {
        const float psychoMin = 0.15f; // light psycho-damp when bright
        const float psychoMax = 0.6f;  // stronger when dark
        const float psychoDamping = psychoMin + (psychoMax - psychoMin) * darkness;

        extraDampL[i].setDamping(psychoDamping);
    }

    // LFO parameters
    lfoParameters.frequency_Hz = parameters.modRate;
    lfoParameters.depth        = parameters.modDepth;
    lfo.setParameters(lfoParameters);
}


//==============================================================================

void HybridPlate::applyFDNFeedbackMatrix(const float in[fdnCount],
                                         float (&out)[fdnCount]) const
{
    for (int i = 0; i < fdnCount; ++i)
    {
        float sum = 0.0f;
        for (int j = 0; j < fdnCount; ++j)
            sum += feedbackMatrix[i][j] * in[j];

        // scale down a bit to avoid perfectly lossless circulation
        out[i] = feedbackMatrixScale * sum;
    }
}

//==============================================================================

void HybridPlate::processBlock(juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    auto* left  = buffer.getWritePointer(0);
    auto* right = (numChannels > 1 ? buffer.getWritePointer(1) : nullptr);

    // Snap parameters once per block
    const float mix      = parameters.mix;
    const float dryMix   = 1.0f - mix;
    const float decaySec = juce::jlimit(0.1f, 20.0f,  parameters.decayTime);
    const float roomSize = juce::jlimit(0.25f, 1.75f, parameters.roomSize);
    const float modDepth = parameters.modDepth;

    const float effectiveLoopTime = estimatedLoopTimeSeconds * roomSize;

    // --- Plate decay stretch (how "big" the plate feels) ---
    constexpr float plateDecayStretch = 4.0f;

    // Use stretched decay time so max knob is "bigger" than a tiny tank
    const float scaledDecaySec = decaySec * plateDecayStretch;

    // Canonical RT60 feedback for pure delay: g = exp(ln(0.001) * T_loop / T60)
    const float fbRaw = std::exp(std::log(0.001f) * (effectiveLoopTime / scaledDecaySec));

    // Safety + clamp
    constexpr float feedbackSafety = 0.995f;
    float feedbackGain = fbRaw * feedbackSafety;
    feedbackGain = juce::jlimit(0.0f, 0.995f, feedbackGain);


    const float slew = 0.001f; // modulation slew

    for (int n = 0; n < numSamples; ++n)
    {
        const float dryL = left[n];
        const float dryR = (right ? right[n] : dryL);

        //===========================
        // PRE-DELAY (wet path only)
        //===========================
        preDelayL.pushSample(0, dryL);
        preDelayR.pushSample(0, dryR);

        float inL = preDelayL.readFractional(0, preDelaySamples);
        float inR = preDelayR.readFractional(0, preDelaySamples);

        channelInput[0] = inL;
        channelInput[1] = inR;

        //===========================
        // EARLY DIFFUSION (4 APs / ch)
        //===========================
        float eL = channelInput[0];
        for (int i = 0; i < 4; ++i)
        {
            earlyL[i].pushSample(0, eL);
            eL = earlyL[i].popSample(0);
        }

        float eR = channelInput[1];
        for (int i = 0; i < 4; ++i)
        {
            earlyR[i].pushSample(0, eR);
            eR = earlyR[i].popSample(0);
        }

        const float monoIn = 0.5f * (eL + eR);

        // after computing monoIn
        const float inject[fdnCount] = { 0.7f, -0.5f, 0.5f, -0.7f };
        const float inputGain = 1.1f; // small energy bump into the tank


        //===========================
        // LFO – per-sample
        //===========================
        lfoOutput = lfo.renderAudioOutput();
        const float lfo0  = (float) lfoOutput.normalOutput;
        const float lfo90 = (float) lfoOutput.quadPhaseOutput_pos;

        const float lfoVals[fdnCount] = {
            lfo0,
            lfo90,
            std::tanh(lfo0 + 0.5f * lfo90),
            std::tanh(lfo90 - 0.5f * lfo0)
        };

        const float userModDepth     = parameters.modDepth;
        const float internalModDepth = juce::jlimit(0.0f, 0.5f, userModDepth);

        //===========================
        // Read FDN outputs with modulated delays
        //===========================
        float fdnOut[fdnCount];

        for (int i = 0; i < fdnCount; ++i)
        {
            const float base = juce::jlimit(1.0f, maxDelaySamples[i],
                                            baseDelaySamples[i] * roomSize);

            const float modRatio   = 0.0015f; // 0.15% of base -> subtle plate motion
            const float modSamples = base * modRatio * internalModDepth * lfoVals[i];

            const float targetDelay = juce::jlimit(1.0f, maxDelaySamples[i],
                                                   base + modSamples);

            currentDelaySamples[i] += slew * (targetDelay - currentDelaySamples[i]);
            

            // ---- READ from delay line ----
            float rawOut = fdnLines[i].readFractional(0, currentDelaySamples[i]);

            // ---- Apply high shelf OUTSIDE the loop ----
            float filteredOut = highShelfFilters[i].processSample(rawOut);

            fdnOut[i] = filteredOut;
        }

        //===========================
        // Feedback via FDN matrix
        //===========================
        float mixed[fdnCount];
        applyFDNFeedbackMatrix(fdnOut, mixed);

        float fb[fdnCount];
        for (int i = 0; i < fdnCount; ++i)
            fb[i] = mixed[i] * feedbackGain;

        //===========================
        // Push new input into FDN
        //===========================
        for (int i = 0; i < fdnCount; ++i)
        {
            float newSample = monoIn * inject[i] * inputGain + fb[i];

            float damped = dampingFilters[i].processSample(0, newSample);
            float hp     = hpFDN[i].process(damped);
            float psycho = extraDampL[i].process(hp);

            float softened = psycho; // <-- no high shelf here



            // write into delay line
            fdnLines[i].pushSample(0, softened);
        }



        //===========================
        // Decode FDN to stereo
        //===========================
        float outL = 0.35f * (fdnOut[0] + fdnOut[2]) +
                     0.15f * (fdnOut[1] - fdnOut[3]);

        float outR = 0.35f * (fdnOut[1] + fdnOut[3]) +
                     0.15f * (fdnOut[0] - fdnOut[2]);

        // subtle level lift, ~1 dB
        const float wetGain = 1.12f;
        outL *= wetGain;
        outR *= wetGain;

        channelOutput[0] = outL;
        channelOutput[1] = outR;

        //===========================
        // Final dry/wet mix
        //===========================
        left[n]  = dryMix * dryL + mix * outL;
        if (right)
            right[n] = dryMix * dryR + mix * outR;
    }
}

//==============================================================================

ReverbProcessorParameters& HybridPlate::getParameters()
{
    return parameters;
}

void HybridPlate::setParameters(const ReverbProcessorParameters& params)
{
    if (!(params == parameters))
    {
        parameters = params;
        updateInternalParamsFromUserParams();
    }
}
