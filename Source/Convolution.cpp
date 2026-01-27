#include "Convolution.h"
#include "IRBank.h"

Convolution::Convolution() {}
Convolution::~Convolution() {}

void Convolution::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;
    prepared = true;

    // Reset all state first
    reset();

    // Convolver
    convolver.prepare(spec);

    // Pre-delay
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);

    // Reserve up to 2 seconds of pre-delay
    const int maxPreDelaySamples = (int)(spec.sampleRate * 2.0);
    preDelayL.setMaximumDelayInSamples(maxPreDelaySamples);
    preDelayR.setMaximumDelayInSamples(maxPreDelaySamples);

    updatePreDelay();

    // Filters
    juce::dsp::ProcessSpec filterSpec = spec;
    lowCutL.prepare(filterSpec);
    lowCutR.prepare(filterSpec);
    highCutL.prepare(filterSpec);
    highCutR.prepare(filterSpec);

    updateFilters();

    // Allocate dry buffer with exact size
    dryBuffer.setSize((int)spec.numChannels,
                      (int)spec.maximumBlockSize,
                      false,  // keep existing content: false
                      true,   // clear extra space: true (changed)
                      true);  // avoid realloc if possible
}

void Convolution::reset()
{
    convolver.reset();
    preDelayL.reset();
    preDelayR.reset();
    lowCutL.reset();
    lowCutR.reset();
    highCutL.reset();
    highCutR.reset();
}

void Convolution::updatePreDelay()
{
    if (!prepared)
        return;

    preDelaySamples = parameters.preDelay * 0.001f * (float) currentSampleRate;
    preDelaySamples = juce::jlimit(0.0f, (float) preDelayL.getMaximumDelayInSamples(), preDelaySamples);
}

void Convolution::updateFilters()
{
    if (!prepared)
        return;

    const float sr = (float) currentSampleRate;

    // Ensure low cut is below high cut with proper limits
    const float lowHz  = juce::jlimit(10.0f, sr * 0.45f, parameters.lowCutHz);
    const float highHz = juce::jlimit(lowHz + 10.0f, sr * 0.49f, parameters.highCutHz);

    auto lowCoeffs  = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, lowHz, 0.707f);
    auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, highHz, 0.707f);

    *lowCutL.coefficients  = *lowCoeffs;
    *lowCutR.coefficients  = *lowCoeffs;
    *highCutL.coefficients = *highCoeffs;
    *highCutR.coefficients = *highCoeffs;
}

ConvolutionParameters& Convolution::getParameters()
{
    return parameters;
}

void Convolution::setParameters(const ConvolutionParameters& newParams)
{
    // Check what changed
    bool preDelayChanged =
        (newParams.preDelay != parameters.preDelay);

    bool filtersChanged =
        (newParams.lowCutHz  != parameters.lowCutHz) ||
        (newParams.highCutHz != parameters.highCutHz);

    bool irChanged = ((int)newParams.irIndex != (int)parameters.irIndex);

    parameters = newParams;

    if (preDelayChanged)
        updatePreDelay();

    if (filtersChanged)
        updateFilters();
    
    if (irChanged)
        loadIRAtIndex((int)parameters.irIndex);
}

void Convolution::processBlock(juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);

    if (!prepared)
        return;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Ensure dry buffer has exact size needed
    if (dryBuffer.getNumChannels() != numChannels ||
        dryBuffer.getNumSamples() != numSamples)
    {
        dryBuffer.setSize(numChannels, numSamples, false, true, true);
    }

    // Capture dry signal
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Work on wet path in-place in buffer
    
    // 1) Pre-delay - FIXED: Set delay once before sample loop
    preDelayL.setDelay(preDelaySamples);
    preDelayR.setDelay(preDelaySamples);
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wetData = buffer.getWritePointer(ch);

        for (int n = 0; n < numSamples; ++n)
        {
            const float inSample = wetData[n];

            if (ch == 0)
            {
                preDelayL.pushSample(0, inSample);
                wetData[n] = preDelayL.popSample(0);
            }
            else if (ch == 1)
            {
                preDelayR.pushSample(0, inSample);
                wetData[n] = preDelayR.popSample(0);
            }
            else
            {
                // For channels beyond stereo, pass-through
                wetData[n] = inSample;
            }
        }
    }

    // 2) Tone shaping filters on wet path
    if (numChannels >= 1)
    {
        juce::dsp::AudioBlock<float> block(buffer);

        // Process channel 0
        auto ch0 = block.getSingleChannelBlock(0);
        juce::dsp::ProcessContextReplacing<float> ctx0(ch0);
        lowCutL.process(ctx0);
        highCutL.process(ctx0);

        // Process channel 1 if stereo
        if (numChannels >= 2)
        {
            auto ch1 = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> ctx1(ch1);
            lowCutR.process(ctx1);
            highCutR.process(ctx1);
        }
    }

    // 3) Convolution on wet path
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolver.process(context);
    }

    // 4) Apply IR gain to wet
    const float irGain = juce::Decibels::decibelsToGain(parameters.irGainDb);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wetData = buffer.getWritePointer(ch);
        for (int n = 0; n < numSamples; ++n)
            wetData[n] *= irGain;
    }

    // 5) Dry/wet mix
    const float wetMix  = juce::jlimit(0.0f, 1.0f, parameters.mix);
    const float dryMix  = 1.0f - wetMix;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wetData  = buffer.getWritePointer(ch);
        const auto* dryData  = dryBuffer.getReadPointer(ch);

        for (int n = 0; n < numSamples; ++n)
            wetData[n] = dryMix * dryData[n] + wetMix * wetData[n];
    }
}

// IR loading helpers
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
                                      juce::dsp::Convolution::Trim::yes,
                                      0); // use full IR length
        
        DBG("Convolution::loadIR - Successfully loaded: " + file.getFullPathName());
    }
    catch (const std::exception& e)
    {
        DBG("Convolution::loadIR - Exception loading IR: " + juce::String(e.what()));
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
                                      juce::dsp::Convolution::Trim::yes,
                                      0);
        
        DBG("Convolution::loadIRFromMemory - Successfully loaded IR from memory");
    }
    catch (const std::exception& e)
    {
        DBG("Convolution::loadIRFromMemory - Exception: " + juce::String(e.what()));
    }
}

// IR Bank Management
void Convolution::setIRBank(std::shared_ptr<IRBank> bank)
{
    irBank = bank;
    
    // Load first IR if available
    if (irBank && irBank->getNumIRs() > 0)
    {
        loadIRAtIndex(0);
    }
}

void Convolution::loadIRAtIndex(int index)
{
    if (!irBank)
    {
        DBG("Convolution::loadIRAtIndex - ERROR: No IR bank set");
        return;
    }

    if (!juce::isPositiveAndBelow(index, irBank->getNumIRs()))
    {
        DBG("Convolution::loadIRAtIndex - ERROR: Index out of range: " + juce::String(index));
        return;
    }

    if (index == currentIRIndex)
        return; // Already loaded

    // Explicit bypass IR at index 0
    if (index == 0)
    {
        std::vector<float> impulse(1, 1.0f);

        try
        {
            convolver.loadImpulseResponse(
                impulse.data(),
                impulse.size(),
                juce::dsp::Convolution::Stereo::no,
                juce::dsp::Convolution::Trim::no,
                1);

            DBG("Convolution::loadIRAtIndex - Loaded BYPASS IR (unity impulse)");
            currentIRIndex = 0;
        }
        catch (const std::exception& e)
        {
            DBG("Convolution::loadIRAtIndex - Exception loading bypass IR: " + juce::String(e.what()));
        }
        return;
    }

    // Real IRs for index > 0
    auto irFile = irBank->getIRFile(index);

    if (!irFile.existsAsFile())
    {
        DBG("Convolution::loadIRAtIndex - ERROR: IR file does not exist for index "
            + juce::String(index) + " path: " + irFile.getFullPathName());
        return;
    }

    loadIR(irFile);
    currentIRIndex = index;
}