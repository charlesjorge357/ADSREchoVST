/*
  ==============================================================================

    ReverbPanel.cpp
    Created: 11 Jan 2026 9:16:53pm
    Author:  ferna

  ==============================================================================
*/

#include "ReverbPanel.h"

ReverbPanel::ReverbPanel() {

    setLookAndFeel(&lnf);


    typeDrop.addItem("Type",1);
    typeDrop.setSelectedId(1);
    addAndMakeVisible(typeDrop);
    
    titleLabel.setText("Reverb", juce::dontSendNotification);
	titleLabel.setJustificationType(juce::Justification::centredTop);
    titleLabel.setFont(juce::Font(juce::FontOptions(25.0f,juce::Font::bold | juce::Font::italic)));
	titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd81e5b));
	addAndMakeVisible(titleLabel);


    auto setupLabel = [this](juce::Label& label, const juce::String& name) 
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(17.0f)));
        label.setColour(juce::Label::textColourId, juce::Colour(0xffd81e5b));
        addAndMakeVisible(label);
    };

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters(
            juce::MathConstants<float>::pi * 1.25f,
            juce::MathConstants<float>::pi * 2.75f,
            true);

        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colour(0xffd81e5b));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, juce::Colour(0xff7B1134));

        addAndMakeVisible(s);
    };

    // Sets up value label text box with text editor
    auto setupValueLabel = [this](juce::Label& label)
    {
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::italic)));
        label.setColour(juce::Label::textColourId, juce::Colours::black);
        label.setEditable(true, true, false);

        label.onEditorShow = [&label]()
            {

                // Remove everything that's not part of a number
                auto text = label.getText().retainCharacters("0123456789.-");
                    
                if (auto* editor = label.getCurrentTextEditor())
                {
                    editor->setText(text, false);

                    editor->setInputRestrictions(0, "0123456789.-");
                    editor->setColour(juce::TextEditor::textColourId, juce::Colours::black);
                    editor->applyColourToAllText(juce::Colours::black);
                    editor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::black);
                    editor->setJustification(juce::Justification::centred);
                }

            };

        addAndMakeVisible(label);
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

    setupValueLabel(roomSizeValue);
    setupValueLabel(decayValue);
    setupValueLabel(dampingValue);
    setupValueLabel(modRateValue);
    setupValueLabel(modDepthValue);
    setupValueLabel(preDelayValue);
    setupValueLabel(mixValue);

    //reverbBackground = juce::ImageCache::getFromMemory(BinaryData::reverbBackground_png, BinaryData::reverbBackground_pngSize);

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

    bindValue(roomSize, roomSizeValue, 2);
    bindValue(decay, decayValue, 2, "s");
    bindValue(damping, dampingValue, 0, "Hz");
    bindValue(modRate, modRateValue, 3, "Hz");
    bindValue(modDepth, modDepthValue, 3, "%");
    bindValue(preDelay, preDelayValue, 1, "ms");
    bindValue(Mix, mixValue, 2);
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

    area.removeFromTop(5); // spacing

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

    const int cols = 2;
    const int rows = 4;
    const int cellWidth = area.getWidth() / cols;
    const int cellHeight = area.getHeight() / rows;
   // const int knobSize = 80;

    auto setupKnobInGrid = [&](juce::Label& label,
        juce::Slider& slider,
        juce::Label& valueLabel,
        int col, int row)
        {

            auto cell = juce::Rectangle<int>(
                area.getX() + col * cellWidth,
                area.getY() + row * cellHeight,
                cellWidth,
                cellHeight);

            int knobSize = 80;

            auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);

            slider.setBounds(knobArea);

            label.setBounds(knobArea.getX(), knobArea.getY() - 15,
                knobArea.getWidth(), 15);

            valueLabel.setBounds(knobArea.getX(),
                knobArea.getBottom(),
                knobArea.getWidth(), 15);

        };

    setupKnobInGrid(roomSizeLabel, roomSize, roomSizeValue, 0, 0);
    setupKnobInGrid(decayLabel, decay, decayValue, 1, 0);
    setupKnobInGrid(dampingLabel, damping, dampingValue, 0, 1);
    setupKnobInGrid(modRateLabel, modRate, modRateValue, 1, 1);
    setupKnobInGrid(modDepthLabel, modDepth, modDepthValue, 0, 2);
    setupKnobInGrid(preDelayLabel, preDelay, preDelayValue, 1, 2);
    setupKnobInGrid(mixLabel, Mix, mixValue, 0, 3);





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

ReverbPanel::~ReverbPanel() {
    setLookAndFeel(nullptr);
}
