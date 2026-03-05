/*
  ==============================================================================

    Convolve.cpp
    Created: 11 Jan 2026 7:33:27pm
    Author:  ferna

  ==============================================================================
*/

#include "Convolve.h"

ConvolvePanel::ConvolvePanel() {
    
    titleLabel.setText("Convolve", juce::dontSendNotification);
	titleLabel.setJustificationType(juce::Justification::centredTop);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f,juce::Font::bold | juce::Font::italic)));
	titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
	addAndMakeVisible(titleLabel);

    dropDown.addItem("IR",1);
    dropDown.setSelectedId(1);
    addAndMakeVisible(dropDown);

    /*auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters(
            juce::MathConstants<float>::pi * 1.25f,
            juce::MathConstants<float>::pi * 2.75f,
            true);
        addAndMakeVisible(s);
    };*/


     auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

        auto rotaryParams = s.getRotaryParameters();
        /*rotaryParams.startAngleRadians = juce::MathConstants<float>::pi * 1.5f;
        rotaryParams.endAngleRadians = juce::MathConstants<float>::pi * 3.0f;*/
        //s.setRotaryParameters(rotaryParams);
        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colour(0xff2C4F65));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, juce::Colour(0xffC9DBE7));

        /*s.setRotaryParameters(
            juce::MathConstants<float>::pi * 1.5f,
            juce::MathConstants<float>::pi * 2.75f,
            true);*/

        //s.setLookAndFeel(&svgLookAndFeel);
       // s.setLookAndFeel(&lnk);
        addAndMakeVisible(s);
    };


    setupKnob(preDelaySlider);
    setupKnob(irGainSlider);
    setupKnob(lowCutSlider);
    setupKnob(highCutSlider);
    setupKnob(mixSlider);



            
    auto setupLabel = [this](juce::Label& label, const juce::String& name) 
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
    };


    setupLabel(irGain, "IR Gain");
    setupLabel(lowCut, "Low Cut");
    setupLabel(highCut, "High Cut");
    setupLabel(preDelay,"Pre-Delay");
    setupLabel(mix, "Mix");

    //convoBackground = juce::ImageCache::getFromMemory(BinaryData::convoBackground_png, BinaryData::convoBackground_pngSize);

}

void ConvolvePanel::paint(juce::Graphics& g) {

    //g.drawImage(convoBackground, getLocalBounds().toFloat(),juce::RectanglePlacement::stretchToFit);

    g.fillAll(juce::Colour(0xff669bbc));

    //g.fillAll(juce::Colour(22,23,26));

    //auto panel = getLocalBounds().toFloat().reduced(6.0f);

    //g.setColour(juce::Colour(0xff2B2E33));
    //g.fillRoundedRectangle(panel, 5.0f);

    //g.setColour(juce::Colour(0xff262B31));
    //g.drawRoundedRectangle(panel, 5.0f, 1.5f);
}

void ConvolvePanel::resized()
{
    auto area = getLocalBounds().reduced(12);

    // Title
    titleLabel.setBounds(area.removeFromTop(35));
   
    area.removeFromTop(5); // spacing

    auto comboArea = area.removeFromTop(20);

    dropDown.setBounds(comboArea.reduced(30,0));

    //area.removeFromTop(1); // spacing

    // Divide remaining space into 5 equal rows
    /*int knobHeight = area.getHeight() / 5;

    timeSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    feedbackSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    cutoffSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    rateSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    depthSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));*/

    area.removeFromTop(10);

    int width = 1;
    int height = 5;
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
    
    setKnobInGrid(irGain,irGainSlider,0,0);
    setKnobInGrid(lowCut,lowCutSlider,0,1);
    setKnobInGrid(highCut,highCutSlider,0,2);
    setKnobInGrid(preDelay,preDelaySlider,0,3);
    setKnobInGrid(mix,mixSlider,0,4);


}



