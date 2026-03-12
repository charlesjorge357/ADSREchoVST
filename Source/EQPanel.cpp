/*
  ==============================================================================

    EQPanel.cpp
    Renamed from equalizerPanel.cpp

  ==============================================================================
*/

#include "EQPanel.h"

// -----------------------------------------------------------------------------
// Constructor - layout and styling only, no APVTS knowledge
// -----------------------------------------------------------------------------

EQPanel::EQPanel()
{
    setLookAndFeel(&lnf);

    equalizerLabel.setText("Equalizer", juce::dontSendNotification);
    equalizerLabel.setJustificationType(juce::Justification::centredTop);
    equalizerLabel.setFont(juce::FontOptions(20.0f));
    equalizerLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(equalizerLabel);

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId,
                    juce::Colour(0xfff77f00));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId,
                    juce::Colour(0xff8C4800));
        addAndMakeVisible(s);
    };

    auto setupLabel = [this](juce::Label& label, const juce::String& name)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
    };

    setupKnob(lowFreq);   setupLabel(lowFreqLabel,  "Low Freq ");
    setupKnob(lowGain);   setupLabel(lowGainLabel,  " Low" "\n" "Gain");
    setupKnob(lowQ);      setupLabel(lowQLabel,     "Low Q");
    setupKnob(midFreq);   setupLabel(midFreqLabel,  "Mid Freq");
    setupKnob(midGain);   setupLabel(midGainLabel,  "Mid Gain");
    setupKnob(midQ);      setupLabel(midQLabel,     "Mid Q");
    setupKnob(highFreq);  setupLabel(highFreqLabel, "High" "\n" "Freq");
    setupKnob(highGain);  setupLabel(highGainLabel, " High Gain");
    setupKnob(highQ);     setupLabel(highQLabel,    "High Q");
}

// -----------------------------------------------------------------------------
// attachToAPVTS - wire all 9 sliders
// -----------------------------------------------------------------------------

void EQPanel::attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                             const juce::String& slotID)
{
    auto attach = [&](juce::Slider& s, const juce::String& suffix)
    {
        return std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, slotID + "." + suffix, s);
    };

    lowFreqAttach  = attach(lowFreq,  "eqLowFreq");
    lowGainAttach  = attach(lowGain,  "eqLowGain");
    lowQAttach     = attach(lowQ,     "eqLowQ");
    midFreqAttach  = attach(midFreq,  "eqMidFreq");
    midGainAttach  = attach(midGain,  "eqMidGain");
    midQAttach     = attach(midQ,     "eqMidQ");
    highFreqAttach = attach(highFreq, "eqHighFreq");
    highGainAttach = attach(highGain, "eqHighGain");
    highQAttach    = attach(highQ,    "eqHighQ");
}

// -----------------------------------------------------------------------------
// paint
// -----------------------------------------------------------------------------

void EQPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff003049));
}

// -----------------------------------------------------------------------------
// resized - title row then 9 knobs in a 3x3 grid
// -----------------------------------------------------------------------------

void EQPanel::resized()
{
    auto area = getLocalBounds().reduced(12);

    equalizerLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(15);

    const int cols       = 3;
    const int rows       = 3;
    const int cellWidth  = area.getWidth()  / cols;
    const int cellHeight = area.getHeight() / rows;
    const int knobSize   = 80;

    auto placeKnob = [&](juce::Label& label, juce::Slider& slider,
                         int col, int row)
    {
        auto cell = juce::Rectangle<int>(
            area.getX() + col * cellWidth,
            area.getY() + row * cellHeight,
            cellWidth,
            cellHeight);
        auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);

        slider.setBounds(knobArea);
        
        label.setBounds(knobArea.getX(), knobArea.getY() - 15, knobArea.getWidth(), 25);


        /*label.setBounds(cell.removeFromTop(16));
        slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));*/
    };

    placeKnob(lowFreqLabel,  lowFreq,  0, 0);
    placeKnob(lowGainLabel,  lowGain,  1, 0);
    placeKnob(lowQLabel,     lowQ,     2, 0);
    placeKnob(midFreqLabel,  midFreq,  0, 1);
    placeKnob(midGainLabel,  midGain,  1, 1);
    placeKnob(midQLabel,     midQ,     2, 1);
    placeKnob(highFreqLabel, highFreq, 0, 2);
    placeKnob(highGainLabel, highGain, 1, 2);
    placeKnob(highQLabel,    highQ,    2, 2);
}

EQPanel::~EQPanel() {
    setLookAndFeel(nullptr);
}