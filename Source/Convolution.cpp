#include "Convolution.h"
#include "IRBank.h"

Convolution::Convolution() {}
Convolution::~Convolution() {}

void Convolution::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;
    prepared = true;

    reset();

    convolver.prepare(spec);

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
}

void Convolution::reset()
{
    convolver.reset();
    preDelayL.reset();
    preDelayR.reset();
    lowCut.reset();
    highCut.reset();
    dryWetMixer.reset();
}

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

    // Early-out: nothing changed since last call
    if (lowHz == lastLowCutHz && highHz == lastHighCutHz)
        return;

    lastLowCutHz  = lowHz;
    lastHighCutHz = highHz;

    // Copy coefficient values into the existing allocated objects rather than
    // pointing to newly heap-allocated ones - no allocation on the audio thread
    *lowCut.state  = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, lowHz,  1.0f);
    *highCut.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, highHz, 1.0f);
}

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

    // Guard mix: only call setWetMixProportion when value actually changes
    if (std::abs(newParams.mix - parameters.mix) > 0.001f)
        dryWetMixer.setWetMixProportion(newParams.mix);

    // Guard IR gain: decibelsToGain (std::pow) only runs when value changes
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

void Convolution::processBlock(juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);

    if (!prepared)
        return;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Push dry samples before any wet processing
    dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    // 1) Pre-delay - skipped entirely when inactive
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
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolver.process(context);
    }

    // 3) Tone shaping - two stereo process calls instead of four mono ones
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        lowCut.process(ctx);
        highCut.process(ctx);
    }

    // 4) IR gain - setTargetValue is now called only in setParameters when
    //    irGainDb changes, so here we just apply whatever the smoother holds
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

void Convolution::loadIR(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("Convolution::loadIR - ERROR: File does not exist: " + file.getFullPathName());
        return;
    }

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
}

void Convolution::setIRBank(std::shared_ptr<IRBank> bank)
{
    irBank = bank;

    if (irBank && irBank->getNumIRs() > 0)
        loadIRAtIndex(0);
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

    // Helper lambda to load the bypass impulse (used for fallback below)
    auto loadBypass = [this]()
    {
        reset();
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
    };

    if (index == 0)
    {
        loadBypass();
        DBG("Convolution::loadIRAtIndex - Loaded BYPASS IR");
        currentIRIndex = 0;
        irMissingFlag  = false;
        return;
    }

    // Out-of-range: the user's IR bank has fewer entries than when the preset was saved
    if (!juce::isPositiveAndBelow(index, irBank->getNumIRs()))
    {
        DBG("Convolution::loadIRAtIndex - IR index " + juce::String(index)
            + " out of range (bank has " + juce::String(irBank->getNumIRs())
            + " entries). Falling back to Bypass.");
        loadBypass();
        currentIRIndex = index; // cache the bad index to stop the audio thread retrying every block
        irMissingFlag  = true;
        return;
    }

    auto irFile = irBank->getIRFile(index);

    if (!irFile.existsAsFile())
    {
        DBG("Convolution::loadIRAtIndex - IR file missing for index "
            + juce::String(index) + ": " + irFile.getFullPathName());
        loadBypass();
        currentIRIndex = index; // same: prevent retry loop
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
    currentIRIndex = -1;    // bypass the cached-index guard so loadIRAtIndex always runs
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