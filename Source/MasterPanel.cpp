/*
  ==============================================================================

    MasterPanel.cpp
    Per-chain master mix and gain controls.

  ==============================================================================
*/

#include "MasterPanel.h"

MasterPanel::MasterPanel(int chainIndex)
    : chainIndex(chainIndex)
{
    setLookAndFeel(&lnf);

    titleLabel.setText("Chain " + juce::String(chainIndex + 1),
                       juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredTop);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId,
                    juce::Colour(0xff4A90D9));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId,
                    juce::Colour(0xff1C3D5A));
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

    setupKnob(mixSlider);
    setupKnob(gainSlider);
    setupLabel(mixLabel, "Mix");
    setupLabel(gainLabel, "Gain");
}

MasterPanel::~MasterPanel()
{
    setLookAndFeel(nullptr);
}

void MasterPanel::attachToAPVTS(juce::AudioProcessorValueTreeState& apvts)
{
    const juce::String prefix = "chain_" + juce::String(chainIndex);

    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, prefix + ".masterMix", mixSlider);

    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, prefix + ".gain", gainSlider);
}

void MasterPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Base gradient — lighter steel for contrast with main background
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff3A3E45), 0.0f, 0.0f,
        juce::Colour(0xff2A2E35), 0.0f, bounds.getHeight(),
        false));
    g.fillRect(bounds);

    // Brushed-metal horizontal lines
    juce::Random rng(99);
    for (float y = 0; y < bounds.getHeight(); y += 2.0f)
    {
        float alpha = 0.04f + rng.nextFloat() * 0.05f;
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
    }

    // Subtle border
    g.setColour(juce::Colour(0x30FFFFFF));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);
}

void MasterPanel::resized()
{
    auto area = getLocalBounds().reduced(6);

    titleLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    const int cols = 2;
    const int cellWidth = area.getWidth() / cols;
    const int knobSize = 80;

    auto placeKnob = [&](juce::Label& label, juce::Slider& slider, int col)
    {
        auto cell = juce::Rectangle<int>(
            area.getX() + col * cellWidth,
            area.getY(),
            cellWidth,
            area.getHeight());

        label.setBounds(cell.removeFromTop(16));
        slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
    };

    placeKnob(mixLabel, mixSlider, 0);
    placeKnob(gainLabel, gainSlider, 1);
}
