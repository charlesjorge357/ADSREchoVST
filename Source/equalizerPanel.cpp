/*
  ==============================================================================

    equalizerPanel.cpp
    Created: 20 Feb 2026 8:17:42pm
    Author:  ferna

  ==============================================================================
*/

#include "equalizerPanel.h"

EqualizerPanel::EqualizerPanel() {


    equalizerLabel.setText("Equalizer", juce::dontSendNotification);
    equalizerLabel.setJustificationType(juce::Justification::centredTop);
    equalizerLabel.setFont(juce::FontOptions(20.0f));
    equalizerLabel.setColour(juce::Label::textColourId,juce::Colours::white);
    
    addAndMakeVisible(equalizerLabel);


    auto setUpKnob = [this](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        auto rotaryParams = s.getRotaryParameters();

        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colour(0xff8C4800));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, juce::Colour(0xfff77f00));

        addAndMakeVisible(s);

        };


    setUpKnob(lowFreq);
    setUpKnob(lowGain);
    setUpKnob(lowQ);
    setUpKnob(midFreq);
    setUpKnob(midGain);
    setUpKnob(midQ);
    setUpKnob(highFreq);
    setUpKnob(highGain);
    setUpKnob(highQ);

    auto setUpLabel = [this](juce::Label& label, const juce::String& name) {

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);

     };

    setUpLabel(lowFreqLabel, "Low Freq");
    setUpLabel(lowGainLabel, "Low Gain");
    setUpLabel(lowQLabel, "Low Q");
    setUpLabel(midFreqLabel, "Mid Freq");
    setUpLabel(midGainLabel,"Mid Gain");
    setUpLabel(midQLabel,"Mid Q");
    setUpLabel(highFreqLabel, "High Freq");
    setUpLabel(highGainLabel,"High Gain");
    setUpLabel(highQLabel,"High Q");

    



}

void EqualizerPanel::paint(juce::Graphics& g) {

    g.fillAll(juce::Colour(0xff003049));
}

void EqualizerPanel::resized() {

    auto area = getLocalBounds().reduced(12);


    equalizerLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(10);
    area.removeFromTop(5); // spacing

    int width = 3;
    int height = 3;
    int cellWidth = area.getWidth() / width;
    int cellHeight = area.getHeight() / height;

    auto setKnobInGrid = [&](juce::Label& label,juce::Slider& slider, int col, int row) {
        auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * cellHeight, cellWidth, cellHeight);

        label.setBounds(cell.removeFromTop(1));


        int knobSize = 70;
        auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);
        slider.setBounds(knobArea);

        label.setBounds(knobArea.getX(), knobArea.getY() - 18, knobArea.getWidth(), 15);


     };

    setKnobInGrid(lowFreqLabel,lowFreq,0,0);
    setKnobInGrid(lowGainLabel,lowGain, 1,0);
    setKnobInGrid(lowQLabel,lowQ, 2,0);
    setKnobInGrid(midFreqLabel,midFreq, 0,1);
    setKnobInGrid(midGainLabel,midGain,1,1);
    setKnobInGrid(midQLabel,midQ,2,1);
    setKnobInGrid(highFreqLabel,highFreq,0,2);
    setKnobInGrid(highGainLabel,highGain,1,2);
    setKnobInGrid(highQLabel,highQ,2,2);
}