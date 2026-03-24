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
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters(
            juce::MathConstants<float>::pi * 1.25f,
            juce::MathConstants<float>::pi * 2.75f,
            true);
        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId,
                    juce::Colour(0xfffdf0d5));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId,
                    juce::Colour(0xffB0A795));
        addAndMakeVisible(s);
    };

    auto setupLabel = [this](juce::Label& label, const juce::String& name)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        addAndMakeVisible(label);
    };

    auto setupValueLabel = [this](juce::Label& label)
    {
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::italic)));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setEditable(true, true, false);

        label.onEditorShow = [&label]()
            {

                // Remove everything that's not part of a number
                auto text = label.getText().retainCharacters("0123456789.-");

                if (auto* editor = label.getCurrentTextEditor())
                {
                    editor->setText(text, false);

                    editor->setInputRestrictions(0, "0123456789.-");
                    editor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
                    editor->applyColourToAllText(juce::Colours::white);
                    editor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::black);
                    editor->setJustification(juce::Justification::centred);
                }

            };

        addAndMakeVisible(label);
    };

    setupKnob(threshold);  setupLabel(thresholdLabel, "Threshold");
    setupKnob(ratio);      setupLabel(ratioLabel,     "Ratio");
    setupKnob(attack);     setupLabel(attackLabel,    "Attack");
    setupKnob(release);    setupLabel(releaseLabel,   "Release");
    setupKnob(input);      setupLabel(inputLabel,     "Input");
    setupKnob(output);     setupLabel(outputLabel,    "Output");

    setupValueLabel(thresholdValue);
    setupValueLabel(ratioValue);
    setupValueLabel(attackValue);
    setupValueLabel(releaseValue);
    setupValueLabel(inputValue);
    setupValueLabel(outputValue);
    //setupValueLabel(mixValue);

    auto bindValue = [](juce::Slider& s, juce::Label& l, int decimals = 2, juce::String units = "")
    {
        l.setText(juce::String(s.getValue(), decimals) + " " + units,
            juce::dontSendNotification);

        s.onValueChange = [&s, &l, decimals, units]()
        {
            l.setText(juce::String(s.getValue(), decimals) + " " + units,
                juce::dontSendNotification);
        };

        l.onEditorHide = [&s, &l, decimals, units]()
        {
            auto text = l.getText().retainCharacters("0123456789.-");

            double value;

            if (text.isEmpty())
            {
                value = s.getValue();
            }
            else
            {
                value = text.getDoubleValue();
            }


            value = juce::jlimit(s.getMinimum(), s.getMaximum(), value);
            value = s.snapValue(value, juce::Slider::DragMode::notDragging);

            s.setValue(value);

            l.setText(juce::String(value, decimals) + " " + units,
                juce::dontSendNotification);
        };
    };

    bindValue(threshold, thresholdValue, 1, "dB");
    bindValue(ratio, ratioValue, 1);
    ratioValue.setText(juce::String(ratio.getValue(), 1) + ":1", juce::dontSendNotification);
    ratio.onValueChange = [this]()
    {
        ratioValue.setText(juce::String(ratio.getValue(), 1) + ":1", juce::dontSendNotification);
    };
    bindValue(attack, attackValue, 1, "ms");
    bindValue(release, releaseValue, 0, "ms");
    bindValue(input, inputValue, 1, "dB");
    bindValue(output, outputValue, 1, "dB");
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
    area.removeFromTop(5);

    const int cols       = 3;
    const int rows       = 2;
    const int cellWidth  = area.getWidth()  / cols;
    const int cellHeight = area.getHeight() / rows;
    const int knobSize   = 80;

    auto placeKnob = [&](juce::Label& label, juce::Slider& slider, juce::Label& valueLabel,
                         int col, int row)
    {
        auto cell = juce::Rectangle<int>(
            area.getX() + col * cellWidth,
            area.getY() + row * cellHeight,
            cellWidth,
            cellHeight);

        auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);

        //label.setBounds(cell.removeFromTop(16));
        //slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));
        slider.setBounds(knobArea);

        label.setBounds(knobArea.getX(), knobArea.getY() - 15,
            knobArea.getWidth(), 15);

        valueLabel.setBounds(knobArea.getX(),
            knobArea.getBottom(),
            knobArea.getWidth(), 15);
    };

    placeKnob(thresholdLabel, threshold, thresholdValue, 0, 0);
    placeKnob(ratioLabel,     ratio, ratioValue,     1, 0);
    placeKnob(attackLabel,    attack, attackValue,    2, 0);
    placeKnob(releaseLabel,   release, releaseValue,   0, 1);
    placeKnob(inputLabel,     input, inputValue,     1, 1);
    placeKnob(outputLabel,    output, outputValue,    2, 1);
}

CompressorPanel::~CompressorPanel() {
    setLookAndFeel(nullptr);
}