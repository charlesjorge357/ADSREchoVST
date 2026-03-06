/*
  ==============================================================================

    ReverbPanel.cpp
    Created: 11 Jan 2026 9:16:53pm
    Author:  ferna

  ==============================================================================
*/

#include "ReverbPanel.h"

ReverbPanel::ReverbPanel() {



    typeDrop.addItem("Type",1);
    typeDrop.setSelectedId(1);
    addAndMakeVisible(typeDrop);
    
    titleLabel.setText("Reverb", juce::dontSendNotification);
	titleLabel.setJustificationType(juce::Justification::centredTop);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f,juce::Font::bold | juce::Font::italic)));
	titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd81e5b));
	addAndMakeVisible(titleLabel);


    auto setupLabel = [this](juce::Label& label, const juce::String& name) 
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        label.setColour(juce::Label::textColourId, juce::Colour(0xffd81e5b));
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

        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colour(0xff7B1134));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, juce::Colour(0xffd81e5b));

        addAndMakeVisible(s);
    };

    /*setupKnob(timeSlide);
    setupKnob(feedbackSlider);
    setupKnob(cutoffSlider);
    setupKnob(rateSlider);
    setupKnob(depthSlider);*/

    setupKnob(roomSize);
    setupLabel(roomSizeLabel, "Room Size");

    setupKnob(decay);
    setupLabel(decayLabel,"Decay");

    setupKnob(damping);
    setupLabel(dampingLabel, "Damping");

    setupKnob(modRate);
    setupLabel(modRateLabel, "Mod Rate");

    setupKnob(modDepth);
    setupLabel(modDepthLabel, "Mod Depth");

    //setupKnob(reverbDepth);
    //setupLabel(reverbDepthLabel, "Depth");

    setupKnob(preDelay);
    setupLabel(preDelayLabel, "Pre Delay");

    setupKnob(Mix);
    setupLabel(mixLabel, "Mix");

    //reverbBackground = juce::ImageCache::getFromMemory(BinaryData::reverbBackground_png, BinaryData::reverbBackground_pngSize);


}

void ReverbPanel::attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                                 const juce::String& slotID)
{
    auto attach = [&](juce::Slider& s, const juce::String& suffix) {
        return std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, slotID + "." + suffix, s);
    };

    roomSizeAttach = attach(roomSize,  "roomSize");
    decayAttach    = attach(decay,     "decayTime");
    dampingAttach  = attach(damping,   "damping");
    modRateAttach  = attach(modRate,   "modRate");
    modDepthAttach = attach(modDepth,  "modDepth");
    preDelayAttach = attach(preDelay,  "preDelay");
    mixAttach      = attach(Mix,       "mix");

    auto* typeParam = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(slotID + ".reverbType"));
    typeDrop.clear(juce::dontSendNotification);
    if (typeParam)
        for (int i = 0; i < typeParam->choices.size(); ++i)
            typeDrop.addItem(typeParam->choices[i], i + 1);
    typeAttach = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, slotID + ".reverbType", typeDrop);
}

void ReverbPanel::paint(juce::Graphics& g) {


    //g.drawImage(reverbBackground,getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    //g.fillAll(juce::Colour(22,23,26));
    g.fillAll(juce::Colour(0xfffdf0d5));

    //auto panel = getLocalBounds().toFloat().reduced(6.0f);

    //g.setColour(juce::Colour(0xff2B2E33));
    //g.fillRoundedRectangle(panel, 5.0f);

    //g.setColour(juce::Colour(0xff262B31));
    //g.drawRoundedRectangle(panel, 5.0f, 1.5f);
}

void ReverbPanel::resized()
{
    auto area = getLocalBounds().reduced(12);

    // Title
    titleLabel.setBounds(area.removeFromTop(35));

    area.removeFromTop(10); // spacing

    area.removeFromTop(5);

    auto comboArea = area.removeFromTop(20);

    typeDrop.setBounds(comboArea.reduced(30,0));


    //int columns = 2;
    //int rows = 4;  // 7 knobs = 4 rows (last row has 1 knob)
    //int cellWidth = area.getWidth() / columns;
    //int cellHeight = area.getHeight() / rows;
    //
    //auto setupKnobInGrid = [&](juce::Label& label, juce::Slider& slider, int col, int row)
    //{
    //    auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * cellHeight, cellWidth, cellHeight);
    //    label.setBounds(cell.removeFromTop(15));

    //    int knobSize = 70;  // Change this number for different sizes
    //    slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
    //   // slider.setBounds(cell.reduced(10));
    //};

    int width = 2;
    int height = 4;
    int cellWidth = area.getWidth() / width;
    int cellHeight = area.getHeight() / height;

    auto setupKnobInGrid = [&](juce::Label& label, juce::Slider& slider, int col, int row) {
        auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * cellHeight, cellWidth, cellHeight);

      /*  label.setBounds(cell.removeFromTop(1));*/


        int knobSize = 70;
        auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);
        slider.setBounds(knobArea);

        label.setBounds(knobArea.getX(), knobArea.getY() - 18, knobArea.getWidth(), 15);


        };


    
    setupKnobInGrid(roomSizeLabel, roomSize, 0, 0);
    setupKnobInGrid(decayLabel, decay, 1, 0);
    setupKnobInGrid(dampingLabel, damping, 0, 1);
    setupKnobInGrid(modRateLabel, modRate, 1, 1);
    setupKnobInGrid(modDepthLabel, modDepth, 0, 2);
    //setupKnobInGrid(reverbDepthLabel, reverbDepth, 1, 2); //this is commented out in the header file, so I commented it out here too
    setupKnobInGrid(preDelayLabel, preDelay, 0, 3);  // Centered in last row
    setupKnobInGrid(mixLabel, Mix, 1,3);





    // Divide remaining space into 5 equal rows
    /*int knobHeight = area.getHeight() / 7;

    int rowHeight = knobHeight;*/

    /*timeSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    feedbackSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    cutoffSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    rateSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));
    depthSlider.setBounds(area.removeFromTop(knobHeight).reduced(10));*/

    //auto setupRow = [&](juce::Label& label, juce::Slider& slider)
    //{
    //    auto row = area.removeFromTop(rowHeight);
    //    label.setBounds(row.removeFromTop(20));  // 20px for label

    //    constexpr int knobSize = 48;
    //    auto sliderArea = row;

    //    slider.setBounds(sliderArea.withSizeKeepingCentre(knobSize,knobSize));       // Rest for slider
    //};

    /*roomSize.setBounds(area.removeFromTop(knobHeight).reduced(10));
    decay.setBounds(area.removeFromTop(knobHeight).reduced(10));
    damping.setBounds(area.removeFromTop(knobHeight).reduced(10));
    modRate.setBounds(area.removeFromTop(knobHeight).reduced(10));
    modDepth.setBounds(area.removeFromTop(knobHeight).reduced(10));
    reverbDepth.setBounds(area.removeFromTop(knobHeight).reduced(10));
    preDelay.setBounds(area.removeFromTop(knobHeight).reduced(10));
   */

    /*setupRow(roomSizeLabel, roomSize);
    setupRow(decayLabel, decay);
    setupRow(dampingLabel, damping);
    setupRow(modRateLabel, modRate);
    setupRow(modDepthLabel, modDepth);
    setupRow(reverbDepthLabel, reverbDepth);
    setupRow(preDelayLabel, preDelay);*/
}
