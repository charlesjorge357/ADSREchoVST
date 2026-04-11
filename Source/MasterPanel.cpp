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
    titleLabel.setFont(juce::Font(juce::FontOptions(21.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
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
        label.setFont(juce::Font(juce::FontOptions(17.0f)));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
    };

    auto setupValueLabel = [this](juce::Label& label)
        {
            label.setJustificationType(juce::Justification::centred);
            label.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::italic)));
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

    setupKnob(mixSlider);
    setupKnob(gainSlider);
    setupLabel(mixLabel, "Mix");
    setupLabel(gainLabel, "Gain");
    setupValueLabel(mixValue);
    setupValueLabel(gainValue);

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

    bindValue(mixSlider, mixValue, 2);
    bindValue(gainSlider, gainValue, 2, "dB");


    addAndMakeVisible(enableToggle);

    enableToggle.onClick = [this]
        {
            bool isEnabled = enableToggle.getToggleState();

            setEnabledRecursive(this, isEnabled);

            setAlpha(isEnabled ? 1.0f : 0.5f);

            repaint();
            if (auto* p = getParentComponent())
                p->repaint();
        };


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

    enableToggleAttachment =
        std::make_unique<
        juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts,
            prefix + ".enabled",
            enableToggle);

    auto* enableParam = apvts.getRawParameterValue(prefix + ".enabled");
    if (enableParam != nullptr)
    {
        bool initialState = enableParam->load() > 0.5f;
        setEnabledRecursive(this, initialState);
        setAlpha(initialState ? 1.0f : 0.5f);
    }
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
    enableToggle.setBounds(titleLabel.getRight()-22, titleLabel.getY(), 22, 20);
    area.removeFromTop(7);

    const int cols = 2;
    const int cellWidth = area.getWidth() / cols;
    const int knobSize = 80;

    auto placeKnob = [&](juce::Label& label, juce::Slider& slider, juce::Label& valueLabel, int col)
    {
        auto cell = juce::Rectangle<int>(
            area.getX() + col * cellWidth,
            area.getY(),
            cellWidth,
            area.getHeight());
        auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);

        slider.setBounds(knobArea);

        label.setBounds(knobArea.getX(), knobArea.getY() - 15, knobArea.getWidth(), 25);

        valueLabel.setBounds(knobArea.getX(),
            knobArea.getBottom() - 6,
            knobArea.getWidth(), 15);
    };

    placeKnob(mixLabel, mixSlider, mixValue, 0);
    placeKnob(gainLabel, gainSlider, gainValue, 1);
}

void MasterPanel::setEnabledRecursive(juce::Component* comp, bool shouldBeEnabled)
{
    for (auto* child : comp->getChildren())
    {
        // Don't disable the enable toggle itself, type selector, or remove button
        if (child == &enableToggle)
            continue;

        child->setEnabled(shouldBeEnabled);

        // Recursively disable children
        setEnabledRecursive(child, shouldBeEnabled);
    }
}
