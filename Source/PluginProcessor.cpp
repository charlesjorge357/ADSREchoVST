/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <thread>
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
    
    checkForUpdates();
    
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

    // Cache hot-path param pointers once — avoids String heap allocation in processBlock
    for (int j = 0; j < NUM_CHAINS; j++)
    {
        juce::String prefix = "chain_" + juce::String(j);
        pChainEnabled[j] = apvts.getRawParameterValue(prefix + ".enabled");
        pChainMasterMix[j] = apvts.getRawParameterValue(prefix + ".masterMix");
        pChainGain[j]      = apvts.getRawParameterValue(prefix + ".gain");
    }
}

ADSREchoAudioProcessor::~ADSREchoAudioProcessor()
{
    // Pre-signal all convolver threads to exit in parallel before member destructors
    // call stopBackgroundThread() sequentially (each joins up to 500 ms).
    // Without this, N modules x 2 convolvers x 500 ms = multi-second block on
    // whichever thread calls the destructor (the message thread on FL Studio reload).
    ++(*loadGeneration); // abort any in-flight PATH B background thread early
    for (auto& chain : slots)
        for (auto& slot : chain)
            if (auto* cm = dynamic_cast<ConvolutionModule*>(slot->get()))
                cm->signalConvolversToStop();
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

    // CRITICAL: Clear buffers to prevent garbage data
    masterDryBuffer.clear();
    chainTempBuffer.clear();

    for (auto& chain : slots)
    {
        for (auto& slot : chain)
        {
            slot->prepare(spec);

            // If setStateInformation was called before prepareToPlay (spec.sampleRate
            // was 0 at restore time), the ConvolutionModule was installed without an IR
            // loaded.  Pre-load it now on the message thread so the first processBlock
            // call doesn't do disk I/O on the audio thread while holding the callback lock.
            if (auto* cm = dynamic_cast<ConvolutionModule*>(slot->get()))
            {
                if (!cm->isIRLoaded() && !cm->hasCustomIR())
                {
                    if (auto* p = apvts.getRawParameterValue(slot->slotID + ".convIrIndex"))
                        cm->forceReloadIR((int) p->load());
                }
            }
        }
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
    const juce::ScopedLock sl (getCallbackLock());

    chainTempBuffer.setSize(
        chainTempBuffer.getNumChannels(),
        buffer.getNumSamples(),
        false, false, true);

    masterDryBuffer.setSize(
        masterDryBuffer.getNumChannels(),
        buffer.getNumSamples(),
        false, false, true);
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());


    masterDryBuffer.clear();
    chainTempBuffer.clear();

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
    for (int chainIndex = 0; chainIndex < NUM_CHAINS; chainIndex++)
    {

        chainTempBuffer.clear();

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
            chainTempBuffer.copyFrom(ch, 0, masterDryBuffer, ch, 0, numSamples);


        for (auto& slot : slots[chainIndex])
        {
            slot->process(chainTempBuffer, midiMessages, getPlayHead());
        }

        // ===== Chain mix =====
        const float wet = pChainMasterMix[chainIndex]->load();
        const float dry = 1.0f - wet;

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            auto* wetData = chainTempBuffer.getWritePointer(ch);
            const auto* dryData = masterDryBuffer.getReadPointer(ch);

            // SIMD: wetData = wetData*wet + dryData*dry
            juce::FloatVectorOperations::multiply(wetData, wet, numSamples);
            juce::FloatVectorOperations::addWithMultiply(wetData, dryData, dry, numSamples);
        }

        // ===== Chain gain =====
        const float gainValue = pChainGain[chainIndex]->load();
        chainTempBuffer.applyGain(juce::Decibels::decibelsToGain(gainValue));

        const bool chainEnabled = pChainEnabled[chainIndex]->load() > 0.5f;
        if (!chainEnabled)
            continue;

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            buffer.addFrom(ch, 0, chainTempBuffer, ch, 0, numSamples, 1.0f);
        }

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
juce::ValueTree ADSREchoAudioProcessor::getStateTree()
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

                if (auto* convMod = dynamic_cast<ConvolutionModule*>(mod))
                {
                    if (convMod->hasCustomIR())
                        slot.setProperty("customIRPath", convMod->getCustomIRPath(), nullptr);

                    // Persist bank IR index so loadFromValueTree can pre-load it
                    // in Phase 1 (message thread) rather than lazily on the audio thread.
                    if (auto* p = apvts.getRawParameterValue(slots[j][i]->slotID + ".convIrIndex"))
                        slot.setProperty("irIndex", (int) p->load(), nullptr);
                }

                chain.addChild(slot, -1, nullptr);
            }
        }

        moduleState.addChild(chain, -1, nullptr);
    }

    state.addChild(moduleState, -1, nullptr);
    state.setProperty("currentPresetName", currentPresetName, nullptr);
    return state;
}

void ADSREchoAudioProcessor::loadFromValueTree (const juce::ValueTree& state)
{
    // Phase 1: Build new modules off the audio lock. For ConvolutionModules we
    // only mark the IR path here — actual file I/O and FFT init happen inside
    // prepare() (Phase 2) once we have the correct host block size. This avoids
    // starting background threads with wrong block sizes and doing double I/O.
    struct PendingSlot { int chain, slot; std::unique_ptr<EffectModule> module; int pendingIRIndex = -2; };
    std::vector<PendingSlot> incoming;

    auto modules = state.getChildWithName("Modules");

    // Guard: VST3 hosts call setComponentState on BOTH the audio component AND the
    // edit controller. The controller call may arrive after prepareToPlay with a
    // state that has no Modules subtree (parameter values only). Without this guard
    // that second call would clear all installed modules (doSwap with empty incoming).
    // Preserve existing slots when the Modules child is absent; still sync APVTS
    // and preset name so parameter values stay correct.
    if (!modules.isValid())
    {
        DBG("loadFromValueTree: no Modules child — APVTS-only update, slots preserved");
        currentPresetName = state.getProperty("currentPresetName", "").toString();
        apvts.replaceState(state);
        uiNeedsRebuild.store(true, std::memory_order_release);
        return;
    }

    for (auto chainState : modules)
    {
        int chainIndex = (int)chainState["index"];
        if (!juce::isPositiveAndBelow(chainIndex, NUM_CHAINS))
            continue;

        for (auto slotState : chainState)
        {
            int slotIndex = (int)slotState["index"];
            if (!juce::isPositiveAndBelow(slotIndex, MAX_SLOTS))
                continue;

            auto type = slotState["type"];

            std::unique_ptr<EffectModule> newModule;
            int pendingIRIndex = -2; // -2 = not applicable

            if (type == "Delay")
            {
                newModule = std::make_unique<DelayModule>("null", apvts);
            }
            else if (type == "Reverb")
            {
                newModule = std::make_unique<ReverbModule>("null", apvts);
            }
            else if (type == "Convolution")
            {
                auto module = std::make_unique<ConvolutionModule>("null", apvts);
                module->setIRBank(irBank);

                auto customIRPath = slotState.getProperty("customIRPath", "").toString();
                if (customIRPath.isNotEmpty())
                {
                    juce::File irFile(customIRPath);
                    if (irFile.existsAsFile())
                        module->setCustomIRPathDeferred(irFile); // path only; prepare() loads it
                }
                else
                {
                    // Bank IR: record index so Phase 1 can pre-load it on the message
                    // thread after prepare(), preventing disk I/O on the audio thread
                    // after apvts.replaceState() updates convIrIndex.
                    //
                    // Prefer "irIndex" from the slot XML (written by current getStateTree()).
                    // Fall back to reading convIrIndex directly from the APVTS parameter
                    // properties in the incoming state tree for older saved states that
                    // predate the irIndex slot property. This ensures pendingIRIndex always
                    // matches what apvts.replaceState() will set convIrIndex to, so the
                    // guard in loadIRAtIndex() (index == currentIRIndex → skip) fires and
                    // the audio thread never triggers disk I/O.
                    int irIdxFromXml = (int) slotState.getProperty("irIndex", -1);
                    if (irIdxFromXml >= 0)
                    {
                        pendingIRIndex = irIdxFromXml;
                    }
                    else
                    {
                        // Old state: read the actual parameter value from the APVTS state
                        // tree. APVTS stores unnormalised float values as direct properties
                        // keyed by parameter ID, matching what replaceState() will restore.
                        auto paramKey = slots[chainIndex][slotIndex]->slotID + ".convIrIndex";
                        pendingIRIndex = (int)(float) state.getProperty(paramKey, 0.0f);
                    }
                }

                newModule = std::move(module);
            }
            else if (type == "EQ")
            {
                newModule = std::make_unique<EQModule>("null", apvts);
            }
            else if (type == "Compressor")
            {
                newModule = std::make_unique<CompressorModule>("null", apvts);
            }

            if (newModule)
                incoming.push_back({ chainIndex, slotIndex, std::move(newModule), pendingIRIndex });
        }
    }

    // Set IDs (fast — uses slot->slotID which is stable across calls).
    for (auto& p : incoming)
        p.module->setID(slots[p.chain][p.slot]->slotID);

    // In PATH B the old modules remain active until the new ones are ready.
    // apvts.replaceState() below immediately changes convIrIndex, which would
    // cause setParameters() to call loadIRAtIndex() on the audio thread —
    // disk I/O + FFT on the callback → FL Studio watchdog crash.
    // Suppress that path now; the modules are going to be destroyed anyway.
    if (spec.sampleRate > 0)
    {
        for (auto& chain : slots)
            for (auto& slot : chain)
                if (auto* cm = dynamic_cast<ConvolutionModule*>(slot->get()))
                    cm->suppressIRLoad(true);
    }

    // Restore APVTS state and preset name. This is the primary obligation of
    // setStateInformation() and must complete before we return so that FL Studio
    // does not report "component state wasn't restored".
    currentPresetName = state.getProperty("currentPresetName", "").toString();
    apvts.replaceState(state);
    uiNeedsRebuild.store(true, std::memory_order_release);

    // Two-path swap strategy based on whether the audio engine is running:
    //
    // PATH A (spec.sampleRate == 0): prepareToPlay has not been called yet.
    //   Install bare (unprepared) modules now so prepareToPlay can find and
    //   prepare them. Old modules (if any) are destroyed here. Safe because
    //   no audio is flowing yet.
    //
    // PATH B (spec.sampleRate > 0): prepareToPlay has already been called.
    //   Old modules STAY in the slots and keep producing audio. New modules
    //   are prepared and IR-loaded in a callAsync on the message thread
    //   (outside setStateInformation's timeout window). Only after they are
    //   fully ready are they swapped in under the audio lock. This prevents
    //   the audio thread from ever seeing partially-initialised convolvers.

    auto doSwap = [this](std::vector<PendingSlot>& pendingSlots)
    {
        std::vector<std::unique_ptr<EffectModule>> toDestroy;
        toDestroy.reserve(NUM_CHAINS * MAX_SLOTS * 2);
        {
            const juce::ScopedLock audioLock(getCallbackLock());
            for (auto& chain : slots)
                for (auto& slot : chain)
                    slot->extractAllModules(toDestroy);
            numModules = std::vector<int>(NUM_CHAINS, 0);
            for (auto& p : pendingSlots)
            {
                slots[p.chain][p.slot]->installPreparedModule(std::move(p.module));
                numModules[p.chain]++;
            }
        }
        for (auto& mod : toDestroy)
            if (auto* cm = dynamic_cast<ConvolutionModule*>(mod.get()))
                cm->signalConvolversToStop();
        toDestroy.clear();
    };

    if (spec.sampleRate == 0)
    {
        // PATH A: no audio engine yet — install immediately, prepare later.
        doSwap(incoming);
    }
    else
    {
        // PATH B: audio engine running — prepare fully before swapping.
        // New modules stay in the background thread's capture (invisible to the
        // audio thread) until completely initialised — zero data-race risk.
        //
        // Heavy work (disk I/O + FFT init) runs on a detached background thread
        // so the message thread is never blocked.  Only the fast pointer swap
        // is posted back to the message thread via a nested callAsync.
        const juce::dsp::ProcessSpec capturedSpec = spec;
        const uint64_t myGeneration = ++(*loadGeneration);
        auto genPtr = loadGeneration;   // shared_ptr keeps the atomic alive past processor dtor
        juce::WeakReference<ADSREchoAudioProcessor> weakThis(this);
        std::thread([weakThis, genPtr, capturedSpec, incoming = std::move(incoming), myGeneration]() mutable
        {
            // Prepare and load IRs off the message thread.
            // Check loadGeneration between each module so a newer loadFromValueTree()
            // call (preset switch during loading) causes this thread to exit early
            // instead of spending seconds doing FFT for results that will be discarded.
            for (auto& p : incoming)
            {
                if (genPtr->load(std::memory_order_acquire) != myGeneration)
                    break;

                p.module->prepare(capturedSpec);

                if (genPtr->load(std::memory_order_acquire) != myGeneration)
                    break;

                if (p.pendingIRIndex >= 0)
                    if (auto* cm = dynamic_cast<ConvolutionModule*>(p.module.get()))
                        cm->forceReloadIR(p.pendingIRIndex);
            }

            // Post only the fast pointer swap back to the message thread.
            juce::MessageManager::callAsync([weakThis, incoming = std::move(incoming), myGeneration]() mutable
            {
                auto* self = weakThis.get();
                if (self == nullptr)
                {
                    // Processor was destroyed before this callAsync fired. Signal all
                    // convolver threads to exit in parallel before the serial destructions
                    // below (each stopBackgroundThread() joins at up to 500 ms — doing
                    // them without pre-signaling serialises the waits).
                    for (auto& p : incoming)
                        if (auto* cm = dynamic_cast<ConvolutionModule*>(p.module.get()))
                            cm->signalConvolversToStop();
                    return;
                }

                // A newer loadFromValueTree() has superseded us. Discard our modules
                // without touching the slots — the newer thread owns them now.
                if (myGeneration != self->loadGeneration->load(std::memory_order_acquire))
                {
                    for (auto& p : incoming)
                        if (auto* cm = dynamic_cast<ConvolutionModule*>(p.module.get()))
                            cm->signalConvolversToStop();
                    return;
                }

                // Swap fully-prepared modules into slots under the audio lock (fast).
                std::vector<std::unique_ptr<EffectModule>> toDestroy;
                toDestroy.reserve(self->NUM_CHAINS * self->MAX_SLOTS * 2);
                {
                    const juce::ScopedLock audioLock(self->getCallbackLock());
                    for (auto& chain : self->slots)
                        for (auto& slot : chain)
                            slot->extractAllModules(toDestroy);
                    self->numModules = std::vector<int>(self->NUM_CHAINS, 0);
                    for (auto& p : incoming)
                    {
                        self->slots[p.chain][p.slot]->installPreparedModule(std::move(p.module));
                        self->numModules[p.chain]++;
                    }
                }

                self->uiNeedsRebuild.store(true, std::memory_order_release);

                // Destroy old modules outside the audio lock. Signal convolver
                // threads to exit in parallel before sequential destructions.
                for (auto& mod : toDestroy)
                    if (auto* cm = dynamic_cast<ConvolutionModule*>(mod.get()))
                        cm->signalConvolversToStop();
                toDestroy.clear();
            });
        }).detach();
    }
}

void ADSREchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = getStateTree();
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
    loadFromValueTree(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout ADSREchoAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Master controls
    for (int j = 0; j < NUM_CHAINS; j++)
    {
        // Per Chain Controls (id "chain_1.gain")
        juce::String chainPrefix = "chain_" + juce::String(j);
        layout.add(std::make_unique<juce::AudioParameterFloat>(chainPrefix + ".gain", "Gain",
            juce::NormalisableRange<float>(-12.f, 12.f, .01f, 1.f), 0.f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(chainPrefix + ".masterMix", "Master Mix",
            juce::NormalisableRange<float>(0.f, 1.f, .01f, 1.f), 1.0f));  // Default 100% wet

        layout.add(std::make_unique<juce::AudioParameterBool>(chainPrefix + ".enabled", "Enabled", true));

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

            // 3-Band EQ
            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqLowFreq", "Low Freq",
                juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.4f), 200.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqLowGain", "Low Gain",
                juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqLowQ", "Low Q",
                juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 0.707f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqMidFreq", "Mid Freq",
                juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.4f), 1000.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqMidGain", "Mid Gain",
                juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqMidQ", "Mid Q",
                juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 0.707f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqHighFreq", "High Freq",
                juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.4f), 8000.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqHighGain", "High Gain",
                juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".eqHighQ", "High Q",
                juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 0.707f));

            // Compressor
            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".compThreshold", "Threshold",
                juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f, 0.5f), -18.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".compRatio", "Ratio",
                juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".compAttack", "Attack",
                juce::NormalisableRange<float>(1.0f, 200.0f, 0.1f, 0.5f), 10.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".compRelease", "Release",
                juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.4f), 100.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".compInput", "Comp Input",
                juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));

            layout.add(std::make_unique<juce::AudioParameterFloat>(prefix + ".compOutput", "Comp Output",
                juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
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
    if (effectModule == nullptr)
        return {};
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
                {
                    auto module = std::make_unique<ConvolutionModule>("null", apvts);
                    module->setIRBank(irBank);
                    slot->setModule(std::move(module));
                    break;
                }

                case ModuleType::EQ:
                    slot->setModule(std::make_unique<EQModule>("null", apvts));
                    break;

                case ModuleType::Compressor:
                    slot->setModule(std::make_unique<CompressorModule>("null", apvts));
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
        {
            auto module = std::make_unique<ConvolutionModule>("null", apvts);
            module->setIRBank(irBank);  // ADD THIS!
            toChange->setModule(std::move(module));
            break;
        }

        case ModuleType::EQ:
            toChange->setModule(std::make_unique<EQModule>("null", apvts));
             break;

        case ModuleType::Compressor:
            toChange->setModule(std::make_unique<CompressorModule>("null", apvts));
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


void ADSREchoAudioProcessor::checkForUpdates()
{
    juce::URL versionUrl("https://raw.githubusercontent.com/charlesjorge357/ADSREchoVST/Release/version.json");

    std::thread([this, versionUrl]()
        {
            DBG("Thread started");

            juce::String jsonString = versionUrl.readEntireTextStream();

            DBG("Downloaded JSON:");
            DBG(jsonString);

            if (jsonString.isNotEmpty())
            {
                auto json = juce::JSON::parse(jsonString);

                DBG("Parsed JSON");

                juce::String latestVersion = json["latest_version"].toString();
                juce::String webUrl = json["download_url"].toString();

                DBG("Latest version: " + latestVersion);
                DBG("Current version: " + currentVersion);

                if (latestVersion != currentVersion)
                {
                    DBG("version not matching");

                    // MAKE UI POPUP HERE
                    // webUrl is the link the popup should go to
                }
                else
                {
                    DBG("version matching");
                }
            }
            else
            {
                DBG("JSON string was empty");
            }

        }).detach();
}