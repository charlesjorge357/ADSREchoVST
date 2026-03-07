/*
  ==============================================================================

    CompressorPanel.cpp
    Renamed from Compressor.cpp

  ==============================================================================
*/

#include "CompressorPanel.h"

// -----------------------------------------------------------------------------
// Constructor - layout and styling only, no APVTS knowledge
// -----------------------------------------------------------------------------

CompressorPanel::CompressorPanel()
{
    setLookAndFeel(&lnf);

    titleLabel.setText("Compressor", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredTop);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold | juce::Font::italic)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters(
            juce::MathConstants<float>::pi * 1.25f,
            juce::MathConstants<float>::pi * 2.75f,
            true);
        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId,
                    juce::Colour(0xffB0A795));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId,
                    juce::Colour(0xfffdf0d5));
        addAndMakeVisible(s);
    };

    auto setupLabel = [this](juce::Label& label, const juce::String& name)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        addAndMakeVisible(label);
    };

    setupKnob(threshold);  setupLabel(thresholdLabel, "Threshold");
    setupKnob(ratio);      setupLabel(ratioLabel,     "Ratio");
    setupKnob(attack);     setupLabel(attackLabel,    "Attack");
    setupKnob(release);    setupLabel(releaseLabel,   "Release");
    setupKnob(input);      setupLabel(inputLabel,     "Input");
    setupKnob(output);     setupLabel(outputLabel,    "Output");
}

// -----------------------------------------------------------------------------
// attachToAPVTS - wire all 6 sliders
// -----------------------------------------------------------------------------

void CompressorPanel::attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& slotID)
{
    auto attach = [&](juce::Slider& s, const juce::String& suffix)
    {
        return std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, slotID + "." + suffix, s);
    };

    thresholdAttach = attach(threshold, "compThreshold");
    ratioAttach     = attach(ratio,     "compRatio");
    attackAttach    = attach(attack,    "compAttack");
    releaseAttach   = attach(release,   "compRelease");
    inputAttach     = attach(input,     "compInput");
    outputAttach    = attach(output,    "compOutput");
}

// -----------------------------------------------------------------------------
// paint
// -----------------------------------------------------------------------------

void CompressorPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2BAF90));
}

// -----------------------------------------------------------------------------
// resized - title row, then 6 knobs in a 2x3 grid
// -----------------------------------------------------------------------------

void CompressorPanel::resized()
{
    auto area = getLocalBounds().reduced(12);

    titleLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(15);

    const int cols       = 2;
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

        label.setBounds(cell.removeFromTop(16));
        slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
    };

    placeKnob(thresholdLabel, threshold, 0, 0);
    placeKnob(ratioLabel,     ratio,     1, 0);
    placeKnob(attackLabel,    attack,    0, 1);
    placeKnob(releaseLabel,   release,   1, 1);
    placeKnob(inputLabel,     input,     0, 2);
    placeKnob(outputLabel,    output,    1, 2);
}

CompressorPanel::~CompressorPanel() {
    setLookAndFeel(nullptr);
}