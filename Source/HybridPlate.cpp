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
    const float earlyDelaysMs[4] = { 2.5f, 4.0f, 6.0f, 8.5f };
    const float earlyGain        = 0.72f;   // diffusive but not ringy

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
        32.0f,
        44.0f,
        57.0f,
        70.0f
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
        extraDampL[i].prepare(sampleRate, parameters.damping);


    // high shelf for no ring
    for (int i = 0; i < fdnCount; ++i)
    {
        juce::dsp::IIR::Coefficients<float>::Ptr coeff =
            juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate,
                3000.0f,   // frequency where ringing builds
                0.707f,     // Q
                0.5f        // gain factor < 1.0 removes ringing
            );

        highShelfFilters[i].prepare(spec);

        *highShelfFilters[i].coefficients = *coeff;
        
        highShelfFilters[i].reset();
    }


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

    // Damping filter cutoff
    for (int i = 0; i < fdnCount; ++i)
        dampingFilters[i].setCutoffFrequency(parameters.damping);
    for (int i = 0; i < fdnCount; ++i)
        extraDampL[i].setDamping(parameters.damping);


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

    // RT60-mapped feedback gain with safety factor
    const float effectiveLoopTime = estimatedLoopTimeSeconds * roomSize;
    const float fbRaw             = std::exp(-3.0f * effectiveLoopTime / decaySec);

    constexpr float feedbackSafety = 0.95f;  // global safety margin
    float feedbackGain = fbRaw * feedbackSafety;
    feedbackGain = juce::jlimit(0.0f, 0.90f, feedbackGain);

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

        //===========================
        // Read FDN outputs with modulated delays
        //===========================
        float fdnOut[fdnCount];

        for (int i = 0; i < fdnCount; ++i)
        {
            const float base = juce::jlimit(1.0f, maxDelaySamples[i],
                                            baseDelaySamples[i] * roomSize);

            const float modRatio   = 0.003f; // 0.3% of base -> subtle plate motion
            const float modSamples = base * modRatio * modDepth * lfoVals[i];

            const float targetDelay = juce::jlimit(1.0f, maxDelaySamples[i],
                                                   base + modSamples);

            currentDelaySamples[i] += slew * (targetDelay - currentDelaySamples[i]);

            fdnOut[i] = fdnLines[i].readFractional(0, currentDelaySamples[i]);
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
            // new input to this FDN line: early-diffused monoIn + feedback
            float newSample = monoIn + fb[i];

            // first-order lowpass damping
            float damped = dampingFilters[i].processSample(0, newSample);

            float psycho = extraDampL[i].process(damped);

            // high-shelf to tame metallic ringing
            float softened = highShelfFilters[i].processSample(psycho);


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

        channelOutput[0] = outL;
        channelOutput[1] = outR;

        //===========================
        // Final dry/wet mix
        //===========================
        left[n] = dryMix * dryL + mix * outL;
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
