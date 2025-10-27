/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

//==============================================================================
ADSREchoAudioProcessor::ADSREchoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Initialize effect modules
    delayProcessor = std::make_unique<DelayProcessor>();
    reverbProcessor = std::make_unique<ReverbProcessor>();
    convolutionProcessor = std::make_unique<ConvolutionProcessor>();
    compressorProcessor = std::make_unique<CompressorProcessor>();
    eqProcessor = std::make_unique<EQProcessor>();

    // Initialize system modules
    routingMatrix = std::make_unique<RoutingMatrix>();
    presetManager = std::make_unique<PresetManager>();

    // Load factory presets
    presetManager->loadFactoryPresets();
}

ADSREchoAudioProcessor::~ADSREchoAudioProcessor()
{
}

//==============================================================================
const juce::String ADSREchoAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ADSREchoAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ADSREchoAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ADSREchoAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ADSREchoAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ADSREchoAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ADSREchoAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ADSREchoAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ADSREchoAudioProcessor::getProgramName (int index)
{
    return {};
}

void ADSREchoAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ADSREchoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Prepare DSP spec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Prepare all effect modules
    delayProcessor->prepare(spec);
    reverbProcessor->prepare(spec);
    convolutionProcessor->prepare(spec);
    compressorProcessor->prepare(spec);
    eqProcessor->prepare(spec);

    // Prepare internal buffers
    dryBuffer.setSize(spec.numChannels, samplesPerBlock);
    wetBuffer.setSize(spec.numChannels, samplesPerBlock);

    // Update all parameters from APVTS
    updateAllParameters();
}

void ADSREchoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ADSREchoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ADSREchoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Get tempo from host
    if (auto* playHead = getPlayHead())
    {
        if (auto positionInfo = playHead->getPosition())
        {
            if (auto bpm = positionInfo->getBpm())
            {
                currentTempo = *bpm;
                tempoValid = true;

                // Update delay processor with current tempo
                delayProcessor->setTempo(currentTempo);
            }
            else
            {
                tempoValid = false;
            }
        }
    }

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Store dry signal for parallel processing
    dryBuffer.makeCopyOf(buffer);

    // Route through effect chain based on current topology
    routeSignalChain(buffer);

    // Apply global dry/wet mix
    auto* dryWetParam = apvts.getRawParameterValue(ParameterIDs::dryWetMix);
    if (dryWetParam != nullptr)
    {
        float dryWet = dryWetParam->load();
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* dryData = dryBuffer.getReadPointer(channel);
            auto* wetData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                wetData[sample] = dryData[sample] * (1.0f - dryWet) + wetData[sample] * dryWet;
            }
        }
    }

    // Apply output gain
    auto* outputGainParam = apvts.getRawParameterValue(ParameterIDs::outputGain);
    if (outputGainParam != nullptr)
    {
        float gainDb = outputGainParam->load();
        float gain = juce::Decibels::decibelsToGain(gainDb);
        buffer.applyGain(gain);
    }
}

//==============================================================================
bool ADSREchoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ADSREchoAudioProcessor::createEditor()
{
    return new ADSREchoAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ADSREchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ADSREchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout ADSREchoAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // DELAY PARAMETERS
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::delayTimeLeft, "Delay Time L", juce::NormalisableRange<float>(1.0f, 2000.0f, 1.0f), 500.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::delayTimeRight, "Delay Time R", juce::NormalisableRange<float>(1.0f, 2000.0f, 1.0f), 500.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::delayFeedback, "Delay Feedback", juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::delayMix, "Delay Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::delaySyncEnabled, "Delay Sync", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::delayLowCut, "Delay Low Cut", juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f), 20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::delayHighCut, "Delay High Cut", juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f), 20000.0f));

    // REVERB PARAMETERS
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::reverbSize, "Reverb Size", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::reverbDamping, "Reverb Damping", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::reverbWidth, "Reverb Width", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::reverbMix, "Reverb Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::reverbPreDelay, "Reverb Pre-Delay", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::reverbDecayTime, "Reverb Decay", juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::reverbModulation, "Reverb Modulation", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // CONVOLUTION PARAMETERS
    layout.add(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::convolutionEnabled, "Convolution Enable", false));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        ParameterIDs::convolutionIRIndex, "IR Selection", 0, 10, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::convolutionMix, "Convolution Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::convolutionPreDelay, "Conv Pre-Delay", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::convolutionStretch, "IR Stretch", juce::NormalisableRange<float>(0.5f, 2.0f, 0.01f), 1.0f));

    // COMPRESSOR PARAMETERS
    layout.add(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::compressorEnabled, "Compressor Enable", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::compressorThreshold, "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::compressorRatio, "Ratio", juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f), 4.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::compressorAttack, "Attack", juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::compressorRelease, "Release", juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::compressorKnee, "Knee", juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::compressorMakeup, "Makeup Gain", juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f));

    // EQUALIZER PARAMETERS
    layout.add(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::eqEnabled, "EQ Enable", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::eqLowFreq, "Low Freq", juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f), 80.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::eqLowGain, "Low Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::eqMidFreq, "Mid Freq", juce::NormalisableRange<float>(200.0f, 5000.0f, 1.0f), 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::eqMidGain, "Mid Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::eqMidQ, "Mid Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 0.707f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::eqHighFreq, "High Freq", juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f), 8000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::eqHighGain, "High Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // GLOBAL PARAMETERS
    layout.add(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::masterBypass, "Master Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::outputGain, "Output Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::dryWetMix, "Dry/Wet Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    return layout;
}

//==============================================================================
// Helper methods for signal routing and parameter updates
void ADSREchoAudioProcessor::updateAllParameters()
{
    // Update Reverb Parameters
    if (auto* sizeParam = apvts.getRawParameterValue(ParameterIDs::reverbSize))
        reverbProcessor->setSize(sizeParam->load());

    if (auto* dampingParam = apvts.getRawParameterValue(ParameterIDs::reverbDamping))
    {
        // Damping in APVTS is 0-1, but we might want to use it as decay
        reverbProcessor->setDecay(dampingParam->load());
    }

    if (auto* mixParam = apvts.getRawParameterValue(ParameterIDs::reverbMix))
        reverbProcessor->setMix(mixParam->load());

    if (auto* preDelayParam = apvts.getRawParameterValue(ParameterIDs::reverbPreDelay))
        reverbProcessor->setPreDelay(preDelayParam->load());

    if (auto* widthParam = apvts.getRawParameterValue(ParameterIDs::reverbWidth))
    {
        // Width parameter - could map to diffusion
        reverbProcessor->setDiffusion(widthParam->load());
    }

    // TODO: Add other effect parameter updates (Delay, Compressor, EQ, etc.)
}

void ADSREchoAudioProcessor::updateTempo(double bpm)
{
    if (bpm > 0.0 && bpm <= 300.0)
    {
        currentTempo = bpm;
        tempoValid = true;
        delayProcessor->setTempo(currentTempo);
    }
}

void ADSREchoAudioProcessor::routeSignalChain(juce::AudioBuffer<float>& buffer)
{
    // Route signal through effect chain based on routing matrix
    // This allows flexible effect ordering configured by the user

    auto processingOrder = routingMatrix->getProcessingOrder();

    for (auto effectSlot : processingOrder)
    {
        if (!routingMatrix->isEffectEnabled(effectSlot))
            continue;

        switch (effectSlot)
        {
            case RoutingMatrix::EffectSlot::EQ:
                // eqProcessor->process(buffer);
                break;

            case RoutingMatrix::EffectSlot::Compressor:
                // compressorProcessor->process(buffer);
                break;

            case RoutingMatrix::EffectSlot::Delay:
                // delayProcessor->process(buffer);
                break;

            case RoutingMatrix::EffectSlot::AlgorithmicReverb:
                updateAllParameters();  // Update parameters before processing
                reverbProcessor->process(buffer);
                break;

            case RoutingMatrix::EffectSlot::ConvolutionReverb:
                // convolutionProcessor->process(buffer);
                break;
        }
    }

    // Handle parallel processing if routing matrix specifies it
    // For example, blending algorithmic and convolution reverbs
    if (routingMatrix->getCurrentTopology() == RoutingMatrix::RoutingTopology::ReverbBlend)
    {
        // Implementation for parallel reverb blending will go here
        // wetBuffer.makeCopyOf(dryBuffer);
        // Process algorithmic reverb on buffer
        // Process convolution on wetBuffer
        // Blend based on reverbBlendRatio
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ADSREchoAudioProcessor();
}
