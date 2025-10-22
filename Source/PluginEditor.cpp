/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ADSREchoAudioProcessorEditor::ADSREchoAudioProcessorEditor (ADSREchoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    /*proKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    proKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);*/

   /* gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "Gain", proKnob);

    
    */
    
    for (auto* comp : getComps()) {
        addAndMakeVisible(comp);
    }

   
    setSize (400, 300);
}

ADSREchoAudioProcessorEditor::~ADSREchoAudioProcessorEditor()
{
}

//==============================================================================
void ADSREchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto knobBounds = proKnob.getBounds();

    auto textbounds = knobBounds.translated(0,-35);

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));



    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("A simple Knob!", textbounds, juce::Justification::centred, 1);
}

void ADSREchoAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();

    auto resposiveArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    proKnob.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));

   /* for (auto* comp : getComps()) {
        comp->setBounds(getWidth() / 2 - 50, getHeight() / 2 - 50, 100, 100);
    } */
       


}

std::vector<juce::Component*> ADSREchoAudioProcessorEditor::getComps() {
    return {&proKnob};
}
