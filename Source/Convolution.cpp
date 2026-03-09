#include "Convolution.h"
#include "IRBank.h"

Convolution::Convolution() {}
Convolution::~Convolution() {}

// ===========================================================================
// prepare
// ===========================================================================

void Convolution::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;
    prepared = true;

    reset();

#if USE_CUSTOM_CONVOLVER
    // Choose head/tail partition sizes based on the host block size.
    // The TwoStageFFTConvolver handles arbitrary-length process() calls internally.
    headBlockSize_ = std::max((size_t)spec.maximumBlockSize, (size_t)128);
    tailBlockSize_ = headBlockSize_ * 8;
    monoInBuf.resize(spec.maximumBlockSize);
#else
    convolver.prepare(spec);
#endif

    preDelayL.prepare(spec);
    preDelayR.prepare(spec);

    const int maxPreDelaySamples = (int)(spec.sampleRate * 2.0);
    preDelayL.setMaximumDelayInSamples(maxPreDelaySamples);
    preDelayR.setMaximumDelayInSamples(maxPreDelaySamples);

    updatePreDelay();

    lowCut.prepare(spec);
    highCut.prepare(spec);

    // Invalidate cached freqs so updateFilters runs fully on first call
    lastLowCutHz  = -1.0f;
    lastHighCutHz = -1.0f;
    updateFilters();

    dryWetMixer.prepare(spec);
    dryWetMixer.setWetMixProportion(parameters.mix);

    smoothedIRGain.reset(spec.sampleRate, 0.05);
    smoothedIRGain.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(parameters.irGainDb));

    // prepare() resets the convolvers, wiping any loaded IR.
    // Re-arm currentIRIndex so the next setParameters() / loadIRAtIndex() call
    // actually reloads instead of hitting the cached-index early-return guard.
    const int savedIndex = currentIRIndex;
    currentIRIndex = -1;

    if (customIRActive && customIRPath.isNotEmpty())
        loadIR(juce::File(customIRPath));
    else if (savedIndex >= 0)
        loadIRAtIndex(savedIndex);
}

// ===========================================================================
// reset
// ===========================================================================

void Convolution::reset()
{
#if USE_CUSTOM_CONVOLVER
    convolverL.reset();
    convolverR.reset();
#else
    convolver.reset();
#endif

    preDelayL.reset();
    preDelayR.reset();
    lowCut.reset();
    highCut.reset();
    dryWetMixer.reset();
}

// ===========================================================================
// updatePreDelay / updateFilters
// ===========================================================================

void Convolution::updatePreDelay()
{
    if (!prepared)
        return;

    float newDelay = parameters.preDelay * 0.001f * (float)currentSampleRate;
    newDelay = juce::jlimit(0.0f, (float)preDelayL.getMaximumDelayInSamples(), newDelay);

    if (std::abs(newDelay - preDelaySamples) > 0.01f)
    {
        preDelaySamples   = newDelay;
        preDelayL.setDelay(preDelaySamples);
        preDelayR.setDelay(preDelaySamples);
        isPreDelayActive  = (preDelaySamples > 0.1f);
    }
}

void Convolution::updateFilters()
{
    if (!prepared)
        return;

    const float sr     = (float)currentSampleRate;
    const float lowHz  = juce::jlimit(10.0f, sr * 0.45f, parameters.lowCutHz);
    const float highHz = juce::jlimit(lowHz + 10.0f, sr * 0.49f, parameters.highCutHz);

    if (lowHz == lastLowCutHz && highHz == lastHighCutHz)
        return;

    lastLowCutHz  = lowHz;
    lastHighCutHz = highHz;

    *lowCut.state  = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, lowHz,  1.0f);
    *highCut.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, highHz, 1.0f);
}

// ===========================================================================
// getParameters / setParameters
// ===========================================================================

ConvolutionParameters& Convolution::getParameters()
{
    return parameters;
}

void Convolution::setParameters(const ConvolutionParameters& newParams)
{
    const int oldIRIndex = parameters.irIndex;

    bool preDelayChanged = std::abs(newParams.preDelay  - parameters.preDelay)  > 0.1f;
    bool filtersChanged  = std::abs(newParams.lowCutHz  - parameters.lowCutHz)  > 1.0f
                        || std::abs(newParams.highCutHz - parameters.highCutHz) > 1.0f;
    bool irChanged       = (newParams.irIndex != oldIRIndex);

    if (std::abs(newParams.mix - parameters.mix) > 0.001f)
        dryWetMixer.setWetMixProportion(newParams.mix);

    if (std::abs(newParams.irGainDb - parameters.irGainDb) > 0.01f)
        smoothedIRGain.setTargetValue(juce::Decibels::decibelsToGain(newParams.irGainDb));

    parameters = newParams;

    if (preDelayChanged)
        updatePreDelay();

    if (filtersChanged)
        updateFilters();

    if (irChanged && !customIRActive)
        loadIRAtIndex(newParams.irIndex);
}

// ===========================================================================
// processBlock
// ===========================================================================

void Convolution::processBlock(juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);

    if (!prepared)
        return;

    if (parameters.mix < 0.001f)
        return;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Push dry samples before any wet processing
    dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    // 1) Pre-delay
    if (isPreDelayActive)
    {
        juce::dsp::AudioBlock<float> block(buffer);

        if (numChannels >= 1)
        {
            auto ch0 = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> ctx0(ch0);
            preDelayL.process(ctx0);
        }

        if (numChannels >= 2)
        {
            auto ch1 = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> ctx1(ch1);
            preDelayR.process(ctx1);
        }
    }

    // 2) Convolution
#if USE_CUSTOM_CONVOLVER
    {
        // TwoStageFFTConvolver REQUIRES separate input/output buffers.
        // After the head convolver writes to output, the tail reads from input
        // to fill its delay line. If input==output the tail receives the
        // head-convolved signal instead of the dry signal, producing feedback.
        if (numChannels >= 1)
        {
            std::memcpy(monoInBuf.data(), buffer.getReadPointer(0), (size_t)numSamples * sizeof(float));
            convolverL.process(monoInBuf.data(), buffer.getWritePointer(0), (size_t)numSamples);
        }

        if (numChannels >= 2)
        {
            std::memcpy(monoInBuf.data(), buffer.getReadPointer(1), (size_t)numSamples * sizeof(float));
            convolverR.process(monoInBuf.data(), buffer.getWritePointer(1), (size_t)numSamples);
        }
    }
#else
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolver.process(context);
    }
#endif

    // 3) Tone shaping
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        lowCut.process(ctx);
        highCut.process(ctx);
    }

    // 4) IR gain
    if (smoothedIRGain.isSmoothing())
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int n = 0; n < numSamples; ++n)
                data[n] *= smoothedIRGain.getNextValue();
        }
    }
    else
    {
        const float irGain = smoothedIRGain.getCurrentValue();
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::multiply(buffer.getWritePointer(ch), irGain, numSamples);
    }

    // 5) Dry/wet mix
    dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
}

// ===========================================================================
// IR loading — custom engine helpers
// ===========================================================================

#if USE_CUSTOM_CONVOLVER

// Shared helper — resamples a mono IR vector from srcRate to dstRate
// using a Lagrange interpolator. Returns the original if rates match.
static std::vector<float> resampleIR(std::vector<float> ir,
                                     double srcRate,
                                     double dstRate)
{
    if (std::abs(srcRate - dstRate) < 0.5 || ir.empty())
        return ir;

    const double ratio       = srcRate / dstRate;   // input samples per output sample
    const int    dstLength   = juce::roundToInt((double)ir.size() / ratio);
    if (dstLength <= 0)
        return {};

    std::vector<float> resampled((size_t)dstLength, 0.0f);
    juce::LagrangeInterpolator interp;
    interp.process(ratio, ir.data(), resampled.data(), dstLength);
    return resampled;
}

Convolution::StereoIR Convolution::readStereoIR(const juce::File& file, double targetSampleRate)
{
    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(file));
    if (!reader)
        return {};

    const int    numSamples     = (int)reader->lengthInSamples;
    const double fileSampleRate = reader->sampleRate;
    const int    numCh          = (int)reader->numChannels;

    juce::AudioBuffer<float> buf(numCh, numSamples);
    reader->read(&buf, 0, numSamples, 0, true, true);

    std::vector<float> irL(buf.getReadPointer(0), buf.getReadPointer(0) + numSamples);
    std::vector<float> irR = (numCh >= 2)
        ? std::vector<float>(buf.getReadPointer(1), buf.getReadPointer(1) + numSamples)
        : irL;

    irL = resampleIR(std::move(irL), fileSampleRate, targetSampleRate);
    irR = resampleIR(std::move(irR), fileSampleRate, targetSampleRate);

    return { std::move(irL), std::move(irR) };
}

Convolution::StereoIR Convolution::readStereoIRFromMemory(const void* data, size_t dataSize,
                                                          double targetSampleRate)
{
    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();

    auto stream = std::make_unique<juce::MemoryInputStream>(data, dataSize, false);
    std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(std::move(stream)));
    if (!reader)
        return {};

    const int    numSamples     = (int)reader->lengthInSamples;
    const double fileSampleRate = reader->sampleRate;
    const int    numCh          = (int)reader->numChannels;

    juce::AudioBuffer<float> buf(numCh, numSamples);
    reader->read(&buf, 0, numSamples, 0, true, true);

    std::vector<float> irL(buf.getReadPointer(0), buf.getReadPointer(0) + numSamples);
    std::vector<float> irR = (numCh >= 2)
        ? std::vector<float>(buf.getReadPointer(1), buf.getReadPointer(1) + numSamples)
        : irL;

    irL = resampleIR(std::move(irL), fileSampleRate, targetSampleRate);
    irR = resampleIR(std::move(irR), fileSampleRate, targetSampleRate);

    return { std::move(irL), std::move(irR) };
}

void Convolution::loadCustomEngineIR(const std::vector<float>& irL, const std::vector<float>& irR)
{
    if (irL.empty())
        return;

    const std::vector<float>& useR = irR.empty() ? irL : irR;

    // Energy normalisation: scale = 1 / sqrt(sum of squares across all channels).
    // This matches KlangFalter's "auto gain" approach and gives consistent perceived
    // loudness regardless of IR length or density, unlike peak normalisation which
    // only controls the loudest single sample.
    double sumSq = 0.0;
    for (float s : irL)  sumSq += (double)s * s;
    for (float s : useR) sumSq += (double)s * s;
    const float energy = (float)std::sqrt(sumSq);

    if (energy > 1e-6f)
    {
        const float scale = 1.0f / energy;
        std::vector<float> normL(irL.size());
        std::vector<float> normR(useR.size());
        for (size_t i = 0; i < irL.size();  ++i) normL[i] = irL[i]  * scale;
        for (size_t i = 0; i < useR.size(); ++i) normR[i] = useR[i] * scale;
        convolverL.init(headBlockSize_, tailBlockSize_, normL.data(), normL.size());
        convolverR.init(headBlockSize_, tailBlockSize_, normR.data(), normR.size());
    }
    else
    {
        convolverL.init(headBlockSize_, tailBlockSize_, irL.data(), irL.size());
        convolverR.init(headBlockSize_, tailBlockSize_, useR.data(), useR.size());
    }
}

#endif // USE_CUSTOM_CONVOLVER

// ===========================================================================
// loadIR / loadIRFromMemory
// ===========================================================================

void Convolution::loadIR(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("Convolution::loadIR - ERROR: File does not exist: " + file.getFullPathName());
        return;
    }

#if USE_CUSTOM_CONVOLVER
    auto [irL, irR] = readStereoIR(file, currentSampleRate);
    if (irL.empty())
    {
        DBG("Convolution::loadIR - ERROR: Could not decode: " + file.getFullPathName());
        return;
    }
    loadCustomEngineIR(irL, irR);
    DBG("Convolution::loadIR - Loaded (custom): " + file.getFullPathName());
#else
    try
    {
        convolver.loadImpulseResponse(file,
                                      juce::dsp::Convolution::Stereo::yes,
                                      juce::dsp::Convolution::Trim::no,
                                      0);
        DBG("Convolution::loadIR - Loaded: " + file.getFullPathName());
    }
    catch (const std::exception& e)
    {
        DBG("Convolution::loadIR - Exception: " + juce::String(e.what()));
    }
#endif
}

void Convolution::loadIRFromMemory(const void* data,
                                   size_t dataSize,
                                   double sampleRate,
                                   int numChannels)
{
    juce::ignoreUnused(sampleRate, numChannels);

    if (data == nullptr || dataSize == 0)
    {
        DBG("Convolution::loadIRFromMemory - ERROR: Invalid data or size");
        return;
    }

#if USE_CUSTOM_CONVOLVER
    auto [irL, irR] = readStereoIRFromMemory(data, dataSize, currentSampleRate);
    if (irL.empty())
    {
        DBG("Convolution::loadIRFromMemory - ERROR: Could not decode IR from memory");
        return;
    }
    loadCustomEngineIR(irL, irR);
    DBG("Convolution::loadIRFromMemory - Loaded IR from memory (custom)");
#else
    try
    {
        convolver.loadImpulseResponse(data,
                                      dataSize,
                                      juce::dsp::Convolution::Stereo::yes,
                                      juce::dsp::Convolution::Trim::no,
                                      0);
        DBG("Convolution::loadIRFromMemory - Loaded IR from memory");
    }
    catch (const std::exception& e)
    {
        DBG("Convolution::loadIRFromMemory - Exception: " + juce::String(e.what()));
    }
#endif
}

// ===========================================================================
// IRBank / index loading
// ===========================================================================

void Convolution::setIRBank(std::shared_ptr<IRBank> bank)
{
    irBank = bank;

    // Don't load here — if called before prepare(), the convolvers aren't
    // initialised yet and the load will be wiped when prepare() resets them.
    // prepare() reloads the IR itself after initialising the convolvers.
    // If called after prepare() (e.g. hot-swap), force a reload.
    if (prepared && irBank && irBank->getNumIRs() > 0)
    {
        currentIRIndex = -1;
        loadIRAtIndex(parameters.irIndex);
    }
}

void Convolution::loadIRAtIndex(int index)
{
    if (!irBank)
    {
        DBG("Convolution::loadIRAtIndex - ERROR: No IR bank set");
        return;
    }

    if (index == currentIRIndex)
        return;

    auto loadBypass = [this]()
    {
        reset();

#if USE_CUSTOM_CONVOLVER
        // Unit impulse: convolution with [1.0] is passthrough
        std::vector<float> impulse(1, 1.0f);
        loadCustomEngineIR(impulse, impulse);
#else
        std::vector<float> impulse(1, 1.0f);
        try
        {
            convolver.loadImpulseResponse(
                impulse.data(),
                impulse.size(),
                juce::dsp::Convolution::Stereo::no,
                juce::dsp::Convolution::Trim::no,
                1);
        }
        catch (const std::exception& e)
        {
            DBG("Convolution::loadIRAtIndex - Exception loading bypass IR: " + juce::String(e.what()));
        }
#endif
    };

    if (index == 0)
    {
        loadBypass();
        DBG("Convolution::loadIRAtIndex - Loaded BYPASS IR");
        currentIRIndex = 0;
        irMissingFlag  = false;
        return;
    }

    if (!juce::isPositiveAndBelow(index, irBank->getNumIRs()))
    {
        DBG("Convolution::loadIRAtIndex - IR index " + juce::String(index)
            + " out of range (bank has " + juce::String(irBank->getNumIRs())
            + " entries). Falling back to Bypass.");
        loadBypass();
        currentIRIndex = index;
        irMissingFlag  = true;
        return;
    }

    auto irFile = irBank->getIRFile(index);

    if (!irFile.existsAsFile())
    {
        DBG("Convolution::loadIRAtIndex - IR file missing for index "
            + juce::String(index) + ": " + irFile.getFullPathName());
        loadBypass();
        currentIRIndex = index;
        irMissingFlag  = true;
        return;
    }

    reset();
    loadIR(irFile);
    currentIRIndex = index;
    irMissingFlag  = false;
}

void Convolution::forceLoadIRAtIndex(int index)
{
    currentIRIndex = -1;
    irMissingFlag  = false;
    loadIRAtIndex(index);
}

void Convolution::loadCustomIR(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("Convolution::loadCustomIR - File does not exist: " + file.getFullPathName());
        return;
    }

    reset();
    loadIR(file);
    customIRActive = true;
    customIRPath   = file.getFullPathName();
    currentIRIndex = -1;
    DBG("Convolution::loadCustomIR - Loaded: " + file.getFullPathName());
}

void Convolution::clearCustomIR()
{
    customIRActive = false;
    customIRPath   = {};
    currentIRIndex = -1;
}