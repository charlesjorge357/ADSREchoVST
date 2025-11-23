/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DatorroHall.h"
#include "HybridPlate.h"

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
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    datorroReverb.prepare(spec);
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

    // ===== Read user parameters into reverb =====
    ReverbProcessorParameters params;
    params.roomSize = apvts.getRawParameterValue("RoomSize")->load();
    params.decayTime = apvts.getRawParameterValue("Decay")->load();
    params.damping = apvts.getRawParameterValue("Damping")->load();
    params.modRate = apvts.getRawParameterValue("ModRate")->load();
    params.modDepth = apvts.getRawParameterValue("ModDepth")->load();
    params.mix = apvts.getRawParameterValue("ReverbMix")->load();
    params.preDelay = apvts.getRawParameterValue("PreDelay")->load();
    datorroReverb.setParameters(params);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Store ORIGINAL dry signal for master mix
    juce::AudioBuffer<float> masterDryBuffer;
    masterDryBuffer.makeCopyOf(buffer);
    
    // ============ PER-EFFECT PROCESSING ============
    // Each effect handles its own internal dry/wet
    
    // Effect 1: Reverb (has its own dry/wet via parameters.mix)
    juce::dsp::AudioBlock<float> block(buffer);
    datorroReverb.processBlock(buffer, midiMessages);

    // this is for calling our other algo --v
    // hybridReverb.processBlock(buffer, midiMessages);
    

    // Future: Effect 2: Delay (has its own dry/wet)
    // DelayEffect.process(block);
    
    // Future: Effect 3: Convolution (has its own dry/wet)
    // ConvolutionEffect.process(block);
    
    // ============ MASTER DRY/WET MIX ============
    // Get MASTER mix parameter (0 = original dry, 1 = all processed effects)
    float masterMixValue = apvts.getRawParameterValue("MasterMix")->load();
    
    // Blend original dry with all processed effects
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* processedData = buffer.getWritePointer(channel);
        auto* originalDryData = masterDryBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            processedData[sample] = originalDryData[sample] * (1.0f - masterMixValue) 
                                   + processedData[sample] * masterMixValue;
        }
    }
    
    // Apply master output gain
    float gainValue = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("Gain")->load());
    buffer.applyGain(gainValue);
}

//==============================================================================
bool ADSREchoAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ADSREchoAudioProcessor::createEditor()
{
    //return new ADSREchoAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
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

    // Master controls
    layout.add(std::make_unique<juce::AudioParameterFloat>("Gain", "Gain", 
        juce::NormalisableRange<float>(-6.f, 6.f, .01f, 1.f), 0.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("MasterMix", "Master Mix", 
        juce::NormalisableRange<float>(0.f, 1.f, .01f, 1.f), 1.0f));  // Default 100% wet
    
        // Reverb parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("RoomSize", "Room Size",
        juce::NormalisableRange<float>(0.25f, 1.75f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Decay",
        "Decay Time (s)",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f),
        5.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("PreDelay", "Pre Delay (ms)",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 0.0f));




    layout.add(std::make_unique<juce::AudioParameterFloat>("Damping", "Damping",
        juce::NormalisableRange<float>(500.0f, 18000.0f, 1.f, 0.5f), 8000.0f));

    // This cutoff maps to loopDamping TPT low-pass

    layout.add(std::make_unique<juce::AudioParameterFloat>("ModRate", "Mod Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.001f), 0.30f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("ModDepth", "Mod Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("ReverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f)); 
    
    // Future: Delay, Convolution, etc. each get their own mix
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ADSREchoAudioProcessor();
    //test
}
