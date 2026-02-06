/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DatorroHall.h"

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
    irBank = std::make_shared<IRBank>();

    slots.resize(NUM_CHAINS);
    for (int j = 0; j < NUM_CHAINS; j++)
    {
        for (int i = 0; i < MAX_SLOTS; i++)
        {
            juce::String prefix = "chain_" + juce::String(j) + ".slot_" + juce::String(i);

            slots[j].push_back(std::make_unique<ModuleSlot>(prefix));
        }
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


    // Pre-allocate dry buffer to avoid allocation in processBlock
    masterDryBuffer.setSize(spec.numChannels, samplesPerBlock);
    chainTempBuffer.setSize(spec.numChannels, samplesPerBlock);

    for (auto& chain : slots)
    {
        for (auto& slot : chain)
            slot->prepare(spec);
    }

}

void ADSREchoAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.

    // Free up the pending deleted modules:
    for (auto& chain : slots)
    {
        for (auto& slot : chain)
            slot->destroyPending();
    }
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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());


    // Copy dry signal into pre-allocated buffer (no allocation)
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        masterDryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Move modules around if requested
    if (moveRequested.load(std::memory_order_acquire))
    {
        executeSlotMove();

    }

    buffer.clear();
    // Process the audio through each module slot effect
    bool parallelEnabled = apvts.getRawParameterValue("parallelEnabled")->load();

    for (int chainIndex = 0; chainIndex < NUM_CHAINS - !parallelEnabled; chainIndex++)
    {
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, buffer.getNumSamples());

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
            chainTempBuffer.copyFrom(ch, 0, masterDryBuffer, ch, 0, numSamples);


        for (auto& slot : slots[chainIndex])
        {
            slot->process(chainTempBuffer, midiMessages, getPlayHead());
        }

        // ===== Chain mix =====
        float wet = apvts.getRawParameterValue("chain_" + juce::String(chainIndex) + ".masterMix")->load();
        float dry = 1.0f - wet;

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            auto* wetData = chainTempBuffer.getWritePointer(ch);
            auto* dryData = masterDryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
                wetData[i] = dryData[i] * dry + wetData[i] * wet;
        }

        // ===== Chain gain =====
        float gainValue = apvts.getRawParameterValue("chain_" + juce::String(chainIndex) + ".gain")->load();
        chainTempBuffer.applyGain(juce::Decibels::decibelsToGain(gainValue));

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            buffer.addFrom(ch, 0, chainTempBuffer, ch, 0, numSamples, 1.0f/NUM_CHAINS);
        }

    }
    
    //for (auto& slot : slots) 
    //{
    //    slot->process(buffer, midiMessages, getPlayHead());
    //}

    //// ============ MASTER DRY/WET MIX ============
    //const float masterWet = apvts.getRawParameterValue("MasterMix")->load();
    //const float masterDry = 1.0f - masterWet;

    //// Blend original dry with all processed effects
    //for (int channel = 0; channel < totalNumInputChannels; ++channel)
    //{
    //    auto* processedData = buffer.getWritePointer(channel);
    //    const auto* originalDryData = masterDryBuffer.getReadPointer(channel);

    //    for (int i = 0; i < numSamples; ++i)
    //        processedData[i] = originalDryData[i] * masterDry + processedData[i] * masterWet;
    //}
    //
    //// Apply master output gain
    //float gainValue = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("Gain")->load());
    //buffer.applyGain(gainValue);
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
    auto state = apvts.copyState();

    state.removeChild(state.getChildWithName("Modules"), nullptr);

    juce::ValueTree moduleState("Modules");

    for (int j = 0; j < NUM_CHAINS; j++)
    {
        juce::ValueTree chain("Chain");
        chain.setProperty("index", j, nullptr);
        
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            if (auto* mod = slots[j][i]->get())
            {
                juce::ValueTree slot("Slot");
                slot.setProperty("index", i, nullptr);
                slot.setProperty("type", mod->getType(), nullptr);
                chain.addChild(slot, -1, nullptr);
            }
        }

        moduleState.addChild(chain, -1, nullptr);
    }

    state.addChild(moduleState, -1, nullptr);

    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void ADSREchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (!xml) {
        DBG("no xml!");
        return;
    }
    auto state = juce::ValueTree::fromXml(*xml);

    // Restore Parameters
    apvts.replaceState(state);

    // Clear Modules
    for (auto& chain : slots)
        for(auto& slot : chain)
            slot->clearModule();

    numModules = std::vector<int>(NUM_CHAINS, 0);

    // Restore Topology
    auto modules = state.getChildWithName("Modules");

    for (auto chainState : modules)
    {
        int chainIndex = (int)chainState["index"];

        for (auto slotState : modules)
        {
            int slotIndex = (int)slotState["index"];
            auto type = slotState["type"];

            if (type == "Delay")
            {
                auto module = std::make_unique<DelayModule>("null", apvts);
                slots[chainIndex][slotIndex]->setModule(std::move(module));
            }
            else if (type == "Reverb")
            {
                auto module = std::make_unique<ReverbModule>("null", apvts);
                slots[chainIndex][slotIndex]->setModule(std::move(module));
            }
            else if (type == "Convolution")
            {
                auto module = std::make_unique<ConvolutionModule>("null", apvts);
                module->setIRBank(irBank);
                slots[chainIndex][slotIndex]->setModule(std::move(module));
            }

            numModules[chainIndex]++;
        }
    }

    uiNeedsRebuild.store(true, std::memory_order_release);
}

juce::AudioProcessorValueTreeState::ParameterLayout ADSREchoAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Master controls
    for (int j = 0; j < NUM_CHAINS; j++)
    {
        // Master Controls
        layout.add(std::make_unique<juce::AudioParameterBool>("parallelEnabled", "Parallel Enabled", false));

        // Per Chain Controls (id "chain_1.gain")
        juce::String chainPrefix = "chain_" + juce::String(j);
        layout.add(std::make_unique<juce::AudioParameterFloat>(chainPrefix + ".gain", "Gain",
            juce::NormalisableRange<float>(-6.f, 6.f, .01f, 1.f), 0.f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(chainPrefix + ".masterMix", "Master Mix",
            juce::NormalisableRange<float>(0.f, 1.f, .01f, 1.f), 1.0f));  // Default 100% wet

        // Per Module Controls (id "chain_0.slot_1.mix")
        for (int i = 0; i < MAX_SLOTS; i++)
        {
            juce::String prefix = chainPrefix + ".slot_" + juce::String(i);

            layout.add(std::make_unique<juce::AudioParameterBool>(prefix + ".enabled", "Enabled", true));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".mix", "Mix",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".delayTime", "Delay Time",
                juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.4f), 250.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".feedback", "Feedback",
                juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".roomSize", "Room Size",
                juce::NormalisableRange<float>(0.25f, 1.75f, 0.01f), 1.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".decayTime", "Decay Time (s)",
                juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 5.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".preDelay", "Pre Delay (ms)",
                juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 0.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".damping", "Damping",
                juce::NormalisableRange<float>(500.0f, 10000.0f, 1.f, 0.5f), 8000.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".modRate", "Mod Rate",
                juce::NormalisableRange<float>(0.05f, 5.0f, 0.001f), 0.30f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".modDepth", "Mod Depth",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".convIrIndex", "Conv IR Index",
                juce::NormalisableRange<float>(0.0f, 150.0f, 1.0f), 0.0f));  // adjust max index as needed

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".convIrGain", "Conv IR Gain (dB)",
                juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".convLowCut", "Conv Low Cut (Hz)",
                juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.3f), 80.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".convHighCut", "Conv High Cut (Hz)",
                juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.3f), 12000.0f));

            layout.add(std::make_unique<juce::AudioParameterChoice>(prefix + ".reverbType", "Type",
                juce::StringArray{ "Datorro Hall", "Hybrid Plate" }, 0));

            // Delay BPM Sync
            layout.add(std::make_unique<juce::AudioParameterBool>(prefix + ".delaySyncEnabled", "Delay BPM Sync", false));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".delayBpm", "BPM Override",
                juce::NormalisableRange<float>(20.0f, 300.0f, 0.1f), 120.0f));

            layout.add(std::make_unique<juce::AudioParameterChoice>(prefix + ".delayNoteDiv", "Delay Note Division",
                juce::StringArray{ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                   "1/2 Dotted", "1/4 Dotted", "1/8 Dotted", "1/16 Dotted",
                                   "1/2 Triplet", "1/4 Triplet", "1/8 Triplet", "1/16 Triplet" }, 2));

            // Delay Mode
            layout.add(std::make_unique<juce::AudioParameterChoice>(prefix + ".delayMode", "Delay Mode",
                juce::StringArray{ "Normal", "Ping Pong", "Inverted" }, 0));

            // Delay Pan
            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".delayPan", "Delay Pan",
                juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

            // Delay Filters
            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".delayLowpass", "Delay Lowpass",
                juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".delayHighpass", "Delay Highpass",
                juce::NormalisableRange<float>(20.0f, 5000.0f, 1.0f, 0.3f), 20.0f));
        }
    }

    return layout;
}

int ADSREchoAudioProcessor::getNumSlots() const
{
    return MAX_SLOTS;
}

int ADSREchoAudioProcessor::getNumChannels() const
{
    return NUM_CHAINS;
}

// Returns SlotInfo struct that contains the id, type, and used parameters of the module in a slot
SlotInfo ADSREchoAudioProcessor::getSlotInfo(int chainIndex, int slotIndex)
{
    auto& slot = slots[chainIndex][slotIndex];
    auto effectModule = slot->get();
    return { effectModule->getID(), effectModule->getType(), effectModule->getUsedParameters() };
}

bool ADSREchoAudioProcessor::slotIsEmpty(int chainIndex, int slotIndex)
{
    return !slots[chainIndex][slotIndex]->get();
}

// Add module of moduleType
void ADSREchoAudioProcessor::addModule(int chainIndex, ModuleType moduleType)
{
    if (numModules[chainIndex] == MAX_SLOTS) { return; }

    for (auto& slot : slots[chainIndex]) {
        if (slot->get() == nullptr)
        {
            setSlotDefaults(slot->slotID);
            
            switch (moduleType)
            {
                case ModuleType::Delay:
                    slot->setModule(std::make_unique<DelayModule>("null", apvts));
                    break;

                case ModuleType::Reverb:
                    slot->setModule(std::make_unique<ReverbModule>("null", apvts));
                    break;

                case ModuleType::Convolution:
                    auto module = std::make_unique<ConvolutionModule>("null", apvts);
                    module->setIRBank(irBank);
                    slot->setModule(std::move(module));
                    break;
            }

            numModules[chainIndex]++;
            uiNeedsRebuild.store(true, std::memory_order_release);
            return;
        }   
    }
}

// Remove module at slotIndex
void ADSREchoAudioProcessor::removeModule(int chainIndex, int slotIndex)
{
    auto& toRemove = slots[chainIndex][slotIndex];
    if (toRemove->get() == nullptr)
    {
        DBG("Error: Trying to remove an empty module!");
        return;
    }

    toRemove->clearModule();
    numModules[chainIndex]--;

    requestSlotMove(chainIndex, slotIndex, MAX_SLOTS-1);
}

// Change module at slotIndex to type
void ADSREchoAudioProcessor::changeModuleType(int chainIndex, int slotIndex, ModuleType moduleType)
{
    auto& toChange = slots[chainIndex][slotIndex];
    if (toChange->get() == nullptr)
    {
        DBG("Error: Trying to change an empty module!");
        return;
    }

    std::unique_ptr<EffectModule> newModule;
    switch (moduleType)
    {
        case ModuleType::Delay:
            toChange->setModule(std::make_unique<DelayModule>("null", apvts));
            break;

        case ModuleType::Reverb:
            toChange->setModule(std::make_unique<ReverbModule>("null", apvts));
            break;

        case ModuleType::Convolution:
            auto module = std::make_unique<ConvolutionModule>("null", apvts);
            module->setIRBank(irBank);  // ADD THIS!
            toChange->setModule(std::move(module));
            break;
    }

    uiNeedsRebuild.store(true, std::memory_order_release);

}

// Request that a slot be moved to another position
void ADSREchoAudioProcessor::requestSlotMove(int chainIndex, int from, int to)
{
    pendingMove.chainIndex = chainIndex;
    pendingMove.from = from;
    pendingMove.to = to;
    moveRequested.store(true, std::memory_order_release);
}

void ADSREchoAudioProcessor::executeSlotMove()
{
    const int chainIndex = pendingMove.chainIndex;
    const int from = pendingMove.from;
    const int to = pendingMove.to;

    auto& chain = slots[chainIndex];

    if (juce::isPositiveAndBelow(from, MAX_SLOTS) &&
        juce::isPositiveAndBelow(to, MAX_SLOTS) &&
        from != to)
    {
        auto moved = std::move(chain[from]);

        if (from < to)
        {
            // shift left
            for (int i = from; i < to; ++i)
                chain[i] = std::move(chain[i + 1]);
        }
        else
        {
            // shift right
            for (int i = from; i > to; --i)
                chain[i] = std::move(chain[i - 1]);
        }

        chain[to] = std::move(moved);
    }

    uiNeedsRebuild.store(true, std::memory_order_release);
    moveRequested.store(false, std::memory_order_release);
}

// Reset all parameter values of slot back to default
void ADSREchoAudioProcessor::setSlotDefaults(juce::String slotID)
{
    const auto prefix = slotID + ".";

    for (auto* param : getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            if (p->getParameterID().startsWith(prefix))
            {
                p->setValueNotifyingHost(p->getDefaultValue());
            }
        }
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ADSREchoAudioProcessor();
}
