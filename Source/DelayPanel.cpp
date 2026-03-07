/*
  ==============================================================================

    DelayPanel.cpp
    Created: 5 Jan 2026 12:47:08am
    Author:  ferna

  ==============================================================================
*/


#include "DelayPanel.h"



DelayPanel::DelayPanel() {

    setLookAndFeel(&lnk);

    //auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::Designer_otf, BinaryData::Designer_otfSize);

    /*juce::Font customFont(typeface);
    customFont.setHeight(20.f);*/

    noteDivsion.addItem("Note Divsion",1);
    noteDivsion.addItem("test",2);
    noteDivsion.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey);
    noteDivsion.setColour(juce::ComboBox::outlineColourId, juce::Colours::black);
    noteDivsion.setColour(juce::ComboBox::arrowColourId, juce::Colours::black);
    noteDivsion.setSelectedId(1);

    mode.addItem("Mode",1);
    mode.addItem("test",2);

    addAndMakeVisible(noteDivsion);
    addAndMakeVisible(mode);
    addAndMakeVisible(bmpTog);

    bmpTog.onClick = [this]{updateToggleState(&bmpTog,"BPM Sync");};

    titleLabel.setText("Delay", juce::dontSendNotification);
	titleLabel.setJustificationType(juce::Justification::centredTop);
    //titleLabel.setFont(juce::Font(juce::FontOptions(20.0f,juce::Font::bold | juce::Font::italic)));
    titleLabel.setFont(juce::Font(juce::FontOptions(20.f)));
    //titleLabel.setFont(customFont);
	titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
	addAndMakeVisible(titleLabel);

        

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

        auto rotaryParams = s.getRotaryParameters();
        /*rotaryParams.startAngleRadians = juce::MathConstants<float>::pi * 1.5f;
        rotaryParams.endAngleRadians = juce::MathConstants<float>::pi * 3.0f;*/
        //s.setRotaryParameters(rotaryParams);
        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colour(0xff7B0B13));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, juce::Colour(0xffc1121f));

        /*s.setRotaryParameters(
            juce::MathConstants<float>::pi * 1.5f,
            juce::MathConstants<float>::pi * 2.75f,
            true);*/

        //s.setLookAndFeel(&svgLookAndFeel);
       // s.setLookAndFeel(&lnk);
        addAndMakeVisible(s);
    };

    setupKnob(feedbackSlider);
    setupKnob(timeSlider);
    setupKnob(bpmSlider);
    setupKnob(panSlider);
    setupKnob(lowpassSlider);
    setupKnob(highpassSlider);
    setupKnob(mixSlider);



         
    auto setupLabel = [this](juce::Label& label, const juce::String& name) 
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
    };



    setupLabel(feedbackLabel, "feedback");

    
    setupLabel(timeLabel, "time");

    
    setupLabel(bpmLabel, "BPM");

    
    setupLabel(panLabel, "pan");

    
    setupLabel(lowpassLabel, "lowpass");

    
    setupLabel(highpassLabel, "highpass");
    
    setupLabel(mixLabel, "Mix");



    //delayBackground = juce::ImageCache::getFromMemory(BinaryData::delayBackground_png, BinaryData::delayBackground_pngSize);

}

void DelayPanel::attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotID)
{
    auto attach = [&](juce::Slider& s, const juce::String& suffix) {
        return std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, slotID + "." + suffix, s);
    };

    timeAttach     = attach(timeSlider,     "delayTime");
    feedbackAttach = attach(feedbackSlider, "feedback");
    bpmAttach      = attach(bpmSlider,      "delayBpm");
    panAttach      = attach(panSlider,      "delayPan");
    lowpassAttach  = attach(lowpassSlider,  "delayLowpass");
    highpassAttach = attach(highpassSlider, "delayHighpass");
    mixAttach      = attach(mixSlider,      "mix");

    // Clear dummy items and repopulate from parameter before attaching
    auto* noteDivParam = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(slotID + ".delayNoteDiv"));
    noteDivsion.clear(juce::dontSendNotification);
    if (noteDivParam)
        for (int i = 0; i < noteDivParam->choices.size(); ++i)
            noteDivsion.addItem(noteDivParam->choices[i], i + 1);
    noteDivAttach = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, slotID + ".delayNoteDiv", noteDivsion);

    auto* modeParam = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(slotID + ".delayMode"));
    mode.clear(juce::dontSendNotification);
    if (modeParam)
        for (int i = 0; i < modeParam->choices.size(); ++i)
            mode.addItem(modeParam->choices[i], i + 1);
    modeAttach = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, slotID + ".delayMode", mode);

    syncAttach = std::make_unique<
        juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, slotID + ".delaySyncEnabled", bmpTog);
}

void DelayPanel::updateToggleState(juce::Button* button, juce::String name) {

    auto state = button->getToggleState();

    juce::String stateStrong = state ? "On" : "Off";

}

void DelayPanel::paint(juce::Graphics& g) {

    //g.drawImage(delayBackground, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

    g.fillAll(juce::Colour(0xffc1121f));

   /* auto panel = getLocalBounds().toFloat();

    g.drawRoundedRectangle(panel.reduced(1.0f),10.0f,1.0f);*/

    //g.fillRoundedRectangle(panel,10.0);
   /* g.fillAll(juce::Colour(22,23,26));
  
   

    auto panel = getLocalBounds().toFloat().reduced(6.0f);

    g.setColour(juce::Colour(0xff2B2E33));
    g.fillRoundedRectangle(panel, 5.0f);

    g.setColour(juce::Colour(0xff262B31));
    g.drawRoundedRectangle(panel, 5.0f, 1.5f);*/
}

void DelayPanel::resized()
{
    auto area = getLocalBounds().reduced(12);

    // Title
    titleLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(10);
    area.removeFromTop(5); // spacing

    auto comboArea = area.removeFromTop(20);
    noteDivsion.setBounds(comboArea.reduced(30,0));

    area.removeFromTop(5);

    auto modeArea = area.removeFromTop(20); // Grab another 30px slice from what's left
    mode.setBounds(modeArea.reduced(30, 0));
 

    const int cols = 2;
    const int rows = 4;
    const int cellWidth = area.getWidth() / cols;
    const int cellHeight = area.getHeight() / rows;
    const int knobSize = 80;

    auto setKnobInGrid = [&](juce::Label& label, juce::Slider& slider, int col, int row) {
        auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * cellHeight, cellWidth, cellHeight);
        label.setBounds(cell.removeFromTop(16));
        slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
    };

    setKnobInGrid(feedbackLabel, feedbackSlider, 0, 0);
    setKnobInGrid(timeLabel, timeSlider, 1, 0);

    // BPM cell: toggle at top, then knob
    auto bpmCell = juce::Rectangle<int>(area.getX() + 0 * cellWidth, area.getY() + 1 * cellHeight, cellWidth, cellHeight);
    bmpTog.setBounds(bpmCell.removeFromTop(20));
    bpmLabel.setBounds(bpmCell.removeFromTop(14));
    bpmSlider.setBounds(bpmCell.withSizeKeepingCentre(knobSize, knobSize));

    setKnobInGrid(panLabel, panSlider, 1, 1);
    setKnobInGrid(lowpassLabel, lowpassSlider, 0, 2);
    setKnobInGrid(highpassLabel, highpassSlider, 1, 2);
    setKnobInGrid(mixLabel, mixSlider, 0, 3);

}

DelayPanel::~DelayPanel() {
    setLookAndFeel(nullptr);
}
