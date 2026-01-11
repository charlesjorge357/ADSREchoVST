/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DatorroHall.h"
#include "RoutingMatrix.h"

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
    routingMatrix = std::make_unique<RoutingMatrix>();

    for (int i = 0; i < MAX_SLOTS; i++) {
        juce::String prefix = "slot_" + juce::String(i);

        slots.push_back(std::make_unique<ModuleSlot>(prefix));
    }
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    //routingMatrix->setEffectEnabled(RoutingMatrix::EffectSlot::AlgorithmicReverb, apvts.getRawParameterValue("algoEnabled")->load());

    datorroReverb.prepare(spec);
    hybridReverb.prepare(spec);
    basicDelay.prepare(spec);

    // Pre-allocate dry buffer to avoid allocation in processBlock
    masterDryBuffer.setSize(spec.numChannels, samplesPerBlock);




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

    if (hasPendingChange.exchange(false))
        applyPendingChange();

    for (auto& slot : slots)
        slot->process(buffer, midiMessages);

    /*
    // ===== Read user parameters into reverb =====
    ReverbProcessorParameters params;
    params.roomSize = apvts.getRawParameterValue("RoomSize")->load();
    params.decayTime = apvts.getRawParameterValue("Decay")->load();
    params.damping = apvts.getRawParameterValue("Damping")->load();
    params.modRate = apvts.getRawParameterValue("ModRate")->load();
    params.modDepth = apvts.getRawParameterValue("ModDepth")->load();
    params.mix = apvts.getRawParameterValue("ReverbMix")->load();
    params.preDelay = apvts.getRawParameterValue("PreDelay")->load();


    int algoChoice = apvts.getRawParameterValue("Algorithm")->load();


    // TOGGLE ALGORITHM HERE #1
    // Set parameters for the active algorithm
    if (algoChoice == 0)
        datorroReverb.setParameters(params);
    else
        hybridReverb.setParameters(params);


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Copy dry signal into pre-allocated buffer (no allocation)
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        masterDryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    
    // ============ PER-EFFECT PROCESSING ============
    // Each effect handles its own internal dry/wet

    // Update delay parameters
    basicDelay.setDelayTime(apvts.getRawParameterValue("DelayTime")->load());
    basicDelay.setFeedback(apvts.getRawParameterValue("DelayFeedback")->load());
    basicDelay.setMix(apvts.getRawParameterValue("DelayMix")->load());

    // Enable/disable effects in routing
    bool delayEnabled = apvts.getRawParameterValue("delayEnabled")->load();
    routingMatrix->setEffectEnabled(RoutingMatrix::EffectSlot::Delay, delayEnabled);

    bool algoEnabled = apvts.getRawParameterValue("algoEnabled")->load();
    routingMatrix->setEffectEnabled(RoutingMatrix::EffectSlot::AlgorithmicReverb, algoEnabled);
    routeSignalChain(buffer, midiMessages);




    

    // Future: Effect 2: Delay (has its own dry/wet)
    // DelayEffect.process(block);
    
    // Future: Effect 3: Convolution (has its own dry/wet)
    // ConvolutionEffect.process(block);
    
    // ============ MASTER DRY/WET MIX ============
    const float masterWet = apvts.getRawParameterValue("MasterMix")->load();
    const float masterDry = 1.0f - masterWet;

    // Blend original dry with all processed effects
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* processedData = buffer.getWritePointer(channel);
        const auto* originalDryData = masterDryBuffer.getReadPointer(channel);

        for (int i = 0; i < numSamples; ++i)
            processedData[i] = originalDryData[i] * masterDry + processedData[i] * masterWet;
    }
    
    // Apply master output gain
    float gainValue = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("Gain")->load());
    buffer.applyGain(gainValue);
    */
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

int ADSREchoAudioProcessor::getNumSlots() const
{
    return numModules;
}

SlotInfo ADSREchoAudioProcessor::getSlotInfo(int index) 
{
    auto& slot = slots[index];
    if (!slot) { return {"null", "null"}; }
    return { slot->get()->getID(), slot->get()->getType()};
}

bool ADSREchoAudioProcessor::slotIsEmpty(int index)
{
    return !slots[index]->get();
}

juce::AudioProcessorValueTreeState::ParameterLayout ADSREchoAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Master controls
    layout.add(std::make_unique<juce::AudioParameterFloat>("Gain", "Gain",
        juce::NormalisableRange<float>(-6.f, 6.f, .01f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("MasterMix", "Master Mix",
        juce::NormalisableRange<float>(0.f, 1.f, .01f, 1.f), 1.0f));  // Default 100% wet


    for (int i = 0; i < MAX_SLOTS; i++)
    {
        juce::String prefix = "slot_" + juce::String(i);

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + ".mix",
            "Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
            0.5f
        ));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + ".delay time",
            "Delay Time",
            juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.4f), 
            250.0f
        ));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + ".feedback",
            "Feedback",
            juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 
            0.3f
        ));
    }

    /*
    // Per-effect controls (reverb example)
    layout.add(std::make_unique<juce::AudioParameterBool>("algoEnabled", "Algorithmic Reverb Enabled", true));

    layout.add(std::make_unique<juce::AudioParameterFloat>("ReverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.f, 1.f, .01f, 1.f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("RoomSize", "Room Size",
        juce::NormalisableRange<float>(0.25f, 1.75f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Decay",
        "Decay Time (s)",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),
        5.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("PreDelay", "Pre Delay (ms)",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 0.0f));


    layout.add(std::make_unique<juce::AudioParameterFloat>("Damping", "Damping",
        juce::NormalisableRange<float>(500.0f, 10000.0f, 1.f, 0.5f), 8000.0f));

    // This cutoff maps to loopDamping TPT low-pass

    layout.add(std::make_unique<juce::AudioParameterFloat>("ModRate", "Mod Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.001f), 0.30f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("ModDepth", "Mod Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "Algorithm",            // parameter ID
        "Reverb Algorithm",     // visible name
        juce::StringArray{ "Dattorro Hall", "Hybrid Plate" },
        0                       // default index (0 = Hall)
    ));

    // Convolution Controls
    layout.add(std::make_unique<juce::AudioParameterBool>("convEnabled", "Convolution Reverb Enabled", true));

    layout.add(std::make_unique<juce::AudioParameterFloat>("ConvMix", "Convolution Mix",
        juce::NormalisableRange<float>(0.f, 1.f, .01f, 1.f), 0.5f));

    // Delay Controls
    layout.add(std::make_unique<juce::AudioParameterBool>("delayEnabled", "Delay Enabled", true));

    layout.add(std::make_unique<juce::AudioParameterFloat>("DelayTime", "Delay Time (ms)",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.4f), 250.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("DelayFeedback", "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("DelayMix", "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        */

    return layout;
}

void ADSREchoAudioProcessor::routeSignalChain(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
            basicDelay.processBlock(buffer);
            break;

        case RoutingMatrix::EffectSlot::AlgorithmicReverb:
        {
            juce::dsp::AudioBlock<float> block(buffer);

            int algoChoice = apvts.getRawParameterValue("Algorithm")->load();

            if (algoChoice == 0)
            {
                // Dattorro Hall
                datorroReverb.processBlock(buffer, midiMessages);
            }
            else
            {
                // Hybrid Plate
                hybridReverb.processBlock(buffer, midiMessages);
            }
        }
        break;

        case RoutingMatrix::EffectSlot::ConvolutionReverb:
            // convolutionProcessor->process(buffer);
            break;
        }
    }

    // Handle parallel processing if routing matrix specifies it
    // For example, blending algorithmic and convolution reverbs
}

void ADSREchoAudioProcessor::requestAddModule(ModuleType type)
{
    pendingChange = { PendingChange::Add, type, -1 };
    hasPendingChange.store(true);
}

void ADSREchoAudioProcessor::requestRemoveModule(int slotIndex)
{
    pendingChange = { PendingChange::Remove, ModuleType{}, slotIndex };
    hasPendingChange.store(true);
}

void ADSREchoAudioProcessor::applyPendingChange()
{
    switch (pendingChange.type)
    {
    case PendingChange::Add:
        addModule(pendingChange.moduleType);
        break;

    case PendingChange::Remove:
        //removeModule(pendingChange.slotIndex);
        break;
    }
}

void ADSREchoAudioProcessor::addModule(ModuleType moduleType)
{
    if (numModules == MAX_SLOTS) { return; }

    for (auto& slot : slots) {
        if (slot->get() == nullptr)
        {
            switch (moduleType)
            {
                case ModuleType::Delay:
                    slot->setModule(std::make_unique<DelayModule>(slot->slotID, apvts), spec);
            }
            numModules++;
            return;
        }
        
    }
}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ADSREchoAudioProcessor();
}
