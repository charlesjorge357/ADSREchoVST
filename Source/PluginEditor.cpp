#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ADSREchoAudioProcessorEditor::ADSREchoAudioProcessorEditor(ADSREchoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 300); // Default UI size
}

ADSREchoAudioProcessorEditor::~ADSREchoAudioProcessorEditor() {}

//==============================================================================
void ADSREchoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("ADSREcho", getLocalBounds(), juce::Justification::centred, 1);
}

void ADSREchoAudioProcessorEditor::resized()
{
    // Layout components here
}