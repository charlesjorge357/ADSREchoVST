/*
  ==============================================================================

    ConvolutionPanel.cpp
    Renamed from Convolve.cpp

  ==============================================================================
*/

#include "ConvolutionPanel.h"

// -----------------------------------------------------------------------------
// Constructor - layout and styling only, no APVTS knowledge
// -----------------------------------------------------------------------------

ConvolutionPanel::ConvolutionPanel()
{
    titleLabel.setText("Convolution", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredTop);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold | juce::Font::italic)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // IR selector - items populated in attachToAPVTS once IRBank is available
    dropDownLabel.setText("IR", juce::dontSendNotification);
    dropDownLabel.setJustificationType(juce::Justification::centred);
    dropDownLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    dropDownLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(dropDownLabel);
    addAndMakeVisible(dropDown);
    addAndMakeVisible(browseButton);

    auto setupKnob = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::Rotary);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.setColour(juce::Slider::ColourIds::rotarySliderFillColourId,
                    juce::Colour(0xff2C4F65));
        s.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId,
                    juce::Colour(0xffC9DBE7));
        addAndMakeVisible(s);
    };

    setupKnob(irGainSlider);
    setupKnob(lowCutSlider);
    setupKnob(highCutSlider);
    setupKnob(preDelaySlider);
    setupKnob(mixSlider);

    auto setupLabel = [this](juce::Label& label, const juce::String& name)
    {
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(12.0f)));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
    };

    setupLabel(irGainLabel,   "IR Gain");
    setupLabel(lowCutLabel,   "Low Cut");
    setupLabel(highCutLabel,  "High Cut");
    setupLabel(preDelayLabel, "Pre-Delay");
    setupLabel(mixLabel,      "Mix");
}

// -----------------------------------------------------------------------------
// attachToAPVTS - wire everything to APVTS, populate IR list, hook up browse
// -----------------------------------------------------------------------------

void ConvolutionPanel::attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                                     const juce::String& slotID,
                                     ADSREchoAudioProcessor& processor,
                                     int chainIndex,
                                     int slotIndex)
{
    proc     = &processor;
    chainIdx = chainIndex;
    slotIdx  = slotIndex;

    // Sliders - straightforward attachments
    auto attach = [&](juce::Slider& s, const juce::String& suffix)
    {
        return std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, slotID + "." + suffix, s);
    };

    irGainAttach   = attach(irGainSlider,   "convIrGain");
    lowCutAttach   = attach(lowCutSlider,   "convLowCut");
    highCutAttach  = attach(highCutSlider,  "convHighCut");
    preDelayAttach = attach(preDelaySlider, "preDelay");
    mixAttach      = attach(mixSlider,      "mix");

    // IR dropdown - populate from bank
    auto irBank = processor.getIRBank();
    if (irBank)
        for (int i = 0; i < irBank->getNumIRs(); ++i)
            dropDown.addItem(irBank->getIRName(i), i + 1);

    // Set current selection from parameter value
    auto* irParam = apvts.getRawParameterValue(slotID + ".convIrIndex");
    if (irParam)
        dropDown.setSelectedId((int)irParam->load() + 1,
                               juce::dontSendNotification);

    // If a custom IR is loaded, show its filename instead
    if (auto* mod = dynamic_cast<ConvolutionModule*>(
            processor.slots[chainIndex][slotIndex]->get()))
    {
        if (mod->hasCustomIR())
            dropDown.setText(juce::File(mod->getCustomIRPath())
                                 .getFileNameWithoutExtension(),
                             juce::dontSendNotification);
    }

    // onChange - manual gesture because convIrIndex is a float param
    // driven by a ComboBox rather than a SliderAttachment
    dropDown.onChange = [this, &apvts, slotID]
    {
        const int idx = dropDown.getSelectedId() - 1; // 0-based index
        auto* p = apvts.getParameter(slotID + ".convIrIndex");
        if (p)
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1((float)idx));
            p->endChangeGesture();
        }

        // Selecting a bank IR always clears any custom IR
        if (proc)
            if (auto* mod = dynamic_cast<ConvolutionModule*>(
                    proc->slots[chainIdx][slotIdx]->get()))
                mod->clearCustomIR();
    };

    // Browse button - async file chooser, same logic as ModuleSlotEditor
    browseButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Impulse Response",
            juce::File{},
            "*.wav;*.aif;*.aiff;*.flac");

        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (!file.existsAsFile())
                return;

            if (!proc) return;

            auto* mod = dynamic_cast<ConvolutionModule*>(
                proc->slots[chainIdx][slotIdx]->get());

            if (!mod) return;

            mod->loadCustomIR(file);

            // Show filename in dropdown without triggering onChange
            dropDown.setSelectedId(0, juce::dontSendNotification);
            dropDown.setText(file.getFileNameWithoutExtension(),
                             juce::dontSendNotification);
        });
    };
}

// -----------------------------------------------------------------------------
// paint
// -----------------------------------------------------------------------------

void ConvolutionPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff669bbc));
}

// -----------------------------------------------------------------------------
// resized - title, IR row (dropdown + browse), then 5 knobs in a single column
// -----------------------------------------------------------------------------

void ConvolutionPanel::resized()
{
    auto area = getLocalBounds().reduced(12);

    titleLabel.setBounds(area.removeFromTop(35));
    area.removeFromTop(5);

    // IR selector row: label on left, dropdown fills centre, browse on right
    auto irRow = area.removeFromTop(20);
    dropDownLabel.setBounds(irRow.removeFromLeft(30));
    browseButton.setBounds(irRow.removeFromRight(55));
    dropDown.setBounds(irRow.reduced(4, 0));

    area.removeFromTop(10);

    const int numKnobs  = 5;
    const int cellHeight = area.getHeight() / numKnobs;
    const int knobSize   = 70;

    auto placeKnob = [&](juce::Label& label, juce::Slider& slider, int row)
    {
        auto cell = area.withHeight(cellHeight)
                        .withY(area.getY() + row * cellHeight);
        auto knobArea = cell.withSizeKeepingCentre(knobSize, knobSize);
        slider.setBounds(knobArea);
        label.setBounds(knobArea.getX(),
                        knobArea.getY() - 18,
                        knobArea.getWidth(),
                        15);
    };

    placeKnob(irGainLabel,   irGainSlider,   0);
    placeKnob(lowCutLabel,   lowCutSlider,   1);
    placeKnob(highCutLabel,  highCutSlider,  2);
    placeKnob(preDelayLabel, preDelaySlider, 3);
    placeKnob(mixLabel,      mixSlider,      4);
}