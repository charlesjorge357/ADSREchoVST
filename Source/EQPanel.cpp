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
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
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

    setupKnob(lowFreq);   setupLabel(lowFreqLabel,  "Low Freq ");
    setupKnob(lowGain);   setupLabel(lowGainLabel,  " Low" "\n" "Gain");
    setupKnob(lowQ);      setupLabel(lowQLabel,     "Low Q");
    setupKnob(midFreq);   setupLabel(midFreqLabel,  "Mid Freq");
    setupKnob(midGain);   setupLabel(midGainLabel,  "Mid Gain");
    setupKnob(midQ);      setupLabel(midQLabel,     "Mid Q");
    setupKnob(highFreq);  setupLabel(highFreqLabel, "High" "\n" "Freq");
    setupKnob(highGain);  setupLabel(highGainLabel, " High Gain");
    setupKnob(highQ);     setupLabel(highQLabel,    "High Q");

    setupValueLabel(lowFreqValue);
    setupValueLabel(lowGainValue);
    setupValueLabel(lowQValue);
    setupValueLabel(midFreqValue);
    setupValueLabel(midGainValue);
    setupValueLabel(midQValue);
    setupValueLabel(highFreqValue);
    setupValueLabel(highGainValue);
    setupValueLabel(highQValue);

    // Binds the slider to its label, changing the text value up to a decimal place
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

    bindValue(lowFreq, lowFreqValue, 0, "Hz");
    bindValue(lowGain, lowGainValue, 1, "dB");
    bindValue(lowQ, lowQValue, 2);
    bindValue(midFreq, midFreqValue, 0, "Hz");
    bindValue(midGain, midGainValue, 1, "dB");
    bindValue(midQ, midQValue, 2);
    bindValue(highFreq, highFreqValue, 0, "Hz");
    bindValue(highGain, highGainValue, 1, "dB");
    bindValue(highQ, highQValue, 2);

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
    area.removeFromTop(5);

    const int cols       = 3;
    const int rows       = 3;
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

        slider.setBounds(knobArea);
        
        label.setBounds(knobArea.getX(), knobArea.getY() - 15, knobArea.getWidth(), 25);

        valueLabel.setBounds(knobArea.getX(),
            knobArea.getBottom() + 5,
            knobArea.getWidth(), 15);

        /*label.setBounds(cell.removeFromTop(16));
        slider.setBounds(cell.withSizeKeepingCentre(knobSize, knobSize));*/
    };

    placeKnob(lowFreqLabel,  lowFreq, lowFreqValue, 0, 0);
    placeKnob(lowGainLabel,  lowGain, lowGainValue,  1, 0);
    placeKnob(lowQLabel,     lowQ, lowQValue,     2, 0);
    placeKnob(midFreqLabel,  midFreq, midFreqValue,  0, 1);
    placeKnob(midGainLabel,  midGain, midGainValue,  1, 1);
    placeKnob(midQLabel,     midQ, midQValue,     2, 1);
    placeKnob(highFreqLabel, highFreq, highFreqValue, 0, 2);
    placeKnob(highGainLabel, highGain, highGainValue, 1, 2);
    placeKnob(highQLabel,    highQ, highQValue,    2, 2);
}

EQPanel::~EQPanel() {
    setLookAndFeel(nullptr);
}