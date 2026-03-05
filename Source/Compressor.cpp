/*
  ==============================================================================

    Compressor.cpp
    Created: 28 Feb 2026 12:18:58am
    Author:  ferna

  ==============================================================================
*/

#include "Compressor.h"

Compressor::Compressor() {
    titleLabel.setText("Compressor", juce::dontSendNotification);
	titleLabel.setJustificationType(juce::Justification::centredTop);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f,juce::Font::bold | juce::Font::italic)));
	titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
	addAndMakeVisible(titleLabel);

    auto setupLabel = [this](juce::Label& label, const juce::String& name) 
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        //label.setColour(juce::Label::textColourId, juce::Colour(0xffd81e5b));
        /*label.setColour(juce::Label::backgroundColourId, juce::Colours::black);
        label.setOpaque(true);*/
        addAndMakeVisible(label);
    };


    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters(
            juce::MathConstants<float>::pi * 1.25f,
            juce::MathConstants<float>::pi * 2.75f,
            true);

        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colour(0xffB0A795));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, juce::Colour(0xfffdf0d5));

        addAndMakeVisible(s);
    };


    setupKnob(threshold);
    setupLabel(thresholdLabel,"Threshold");

    setupKnob(ratio);
    setupLabel(ratioLabel,"Ratio");

    setupKnob(attack);
    setupLabel(attackLabel,"Attack");

    setupKnob(release);
    setupLabel(releaseLabel,"Release");

    setupKnob(input);
    setupLabel(inputLabel, "Comp" "\n" "Input");

    setupKnob(output);
    setupLabel(outputLabel, "Comp" "\n" "Output");

}

void Compressor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2BAF90));
}

void Compressor::resized() {

    auto area = getLocalBounds().reduced(12);

    // Title
    titleLabel.setBounds(area.removeFromTop(35));

    area.removeFromTop(10); // spacing

    area.removeFromTop(5);

    auto comboArea = area.removeFromTop(20);

    int width = 2;
    int height = 3;
    int cellWidth = area.getWidth() / width;
    int cellHeight = area.getHeight() / height;


    auto setupKnobInGrid = [&](juce::Label& label, juce::Slider& slider, int col, int row) {
        auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * cellHeight, cellWidth, cellHeight);

      /*  label.setBounds(cell.removeFromTop(1));*/


        int knobSize = 70;
        auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);
        slider.setBounds(knobArea);


        label.setBounds(knobArea.getX(), knobArea.getY() - 18, knobArea.getWidth(), 25);


        };

    setupKnobInGrid(thresholdLabel, threshold, 0,0);
    setupKnobInGrid(ratioLabel, ratio, 1,0);
    setupKnobInGrid(attackLabel, attack, 0,1);
    setupKnobInGrid(releaseLabel, release, 1,1);
    setupKnobInGrid(inputLabel, input, 0,2);
    setupKnobInGrid(outputLabel, output, 1,2);


}
