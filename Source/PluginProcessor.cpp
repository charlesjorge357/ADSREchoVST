#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ADSREchoAudioProcessor::ADSREchoAudioProcessor() {}
ADSREchoAudioProcessor::~ADSREchoAudioProcessor() {}

//==============================================================================
void ADSREchoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP here
}

void ADSREchoAudioProcessor::releaseResources()
{
    // Free any allocated resources
}

void ADSREchoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output if no processing yet
    for (int channel = getTotalNumOutputChannels(); channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

//==============================================================================
bool ADSREchoAudioProcessor::acceptsMidi() const { return false; }
bool ADSREchoAudioProcessor::producesMidi() const { return false; }
bool ADSREchoAudioProcessor::isMidiEffect() const { return false; }
double ADSREchoAudioProcessor::getTailLengthSeconds() const { return 0.0; }

//==============================================================================
int ADSREchoAudioProcessor::getNumPrograms() { return 1; }
int ADSREchoAudioProcessor::getCurrentProgram() { return 0; }
void ADSREchoAudioProcessor::setCurrentProgram(int) {}
const juce::String ADSREchoAudioProcessor::getProgramName(int) { return {}; }
void ADSREchoAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void ADSREchoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Store plugin state
    juce::MemoryOutputStream(destData, true).writeInt(0);
}

void ADSREchoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore plugin state
    juce::MemoryInputStream(data, static_cast<size_t> (sizeInBytes), false).readInt();
}

//==============================================================================
juce::AudioProcessorEditor* ADSREchoAudioProcessor::createEditor()
{
    return new ADSREchoAudioProcessorEditor(*this);
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ADSREchoAudioProcessor();
}