/*
  ==============================================================================

    CompressorModuleSlotEditor.cpp

  ==============================================================================
*/

#include "CompressorModuleSlotEditor.h"

CompressorModuleSlotEditor::CompressorModuleSlotEditor(
    int cIndex,
    int sIndex,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& apvtsRef)
    : BaseModuleSlotEditor(cIndex, sIndex, info, p, apvtsRef)
{
    buildEditor(info);
    startTimerHz(60);
}

// ---------------------------------------------------------------------------
// Build controls from parameter list (identical pattern to EQModuleSlotEditor)
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::buildEditor(const SlotInfo& info)
{
    for (const auto& suffix : info.usedParameters)
    {
        auto id = slotID + "." + suffix;
        auto* param = processor.apvts.getParameter(id);

        if (dynamic_cast<juce::AudioParameterBool*>(param))
            addToggleForParameter(id);
        else if (dynamic_cast<juce::AudioParameterChoice*>(param))
            addChoiceForParameter(id);
        else
            addSliderForParameter(id);
    }

    mod = dynamic_cast<CompressorModule*>(
        processor.slots[chainIndex][slotIndex]->get());

    display = std::make_unique<CompressorDisplayComponent>(*mod);
    addAndMakeVisible(*display);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::layoutEditor(juce::Rectangle<int>& r)
{
    // Meter display on the left - same width as EQ spectrum display
    auto displayArea = r.removeFromLeft(120);
    if (display)
        display->setBounds(displayArea.reduced(5));

    for (int i = 0; i < (int)sliders.size(); ++i)
    {
        auto a = r.removeFromLeft(70);
        sliderLabels[i]->setBounds(a.removeFromBottom(30));
        sliders[i]->setBounds(a);
    }

    for (int i = 0; i < (int)comboBoxes.size(); ++i)
    {
        auto a = r.removeFromLeft(70);
        comboBoxLabels[i]->setBounds(a.removeFromBottom(30));
        comboBoxes[i]->setBounds(a.removeFromTop(25));
    }

    for (int i = 0; i < (int)toggles.size(); ++i)
    {
        auto a = r.removeFromLeft(70);
        toggleLabels[i]->setBounds(a.removeFromBottom(30));
        toggles[i]->setBounds(a.removeFromTop(25));
    }
}

// ---------------------------------------------------------------------------
// Timer - poll meterReady, push values to display (same as EQModuleSlotEditor)
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::timerCallback()
{
    if (mod && mod->meterReady.exchange(false, std::memory_order_acquire))
    {
        display->pushMeterValues(
            mod->inputLevelDb.load(std::memory_order_relaxed),
            mod->gainReductionDb.load(std::memory_order_relaxed));
    }
}

// ---------------------------------------------------------------------------
// Parameter control helpers (identical to EQModuleSlotEditor)
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::addSliderForParameter(const juce::String& id)
{
    auto slider = std::make_unique<juce::Slider>();
    slider->setSliderStyle(juce::Slider::Rotary);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    addAndMakeVisible(*slider);

    auto label = std::make_unique<juce::Label>();
    label->setText(processor.apvts.getParameter(id)->getName(128),
                   juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);

    auto attachment = std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, id, *slider);

    sliders.push_back(std::move(slider));
    sliderAttachments.push_back(std::move(attachment));
    sliderLabels.push_back(std::move(label));
}

void CompressorModuleSlotEditor::addToggleForParameter(const juce::String& id)
{
    auto toggle = std::make_unique<juce::ToggleButton>();
    addAndMakeVisible(*toggle);

    auto label = std::make_unique<juce::Label>();
    label->setText(processor.apvts.getParameter(id)->getName(128),
                   juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);

    auto attachment = std::make_unique<
        juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, id, *toggle);

    toggles.push_back(std::move(toggle));
    toggleAttachments.push_back(std::move(attachment));
    toggleLabels.push_back(std::move(label));
}

void CompressorModuleSlotEditor::addChoiceForParameter(const juce::String& id)
{
    auto combo = std::make_unique<juce::ComboBox>();

    auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.apvts.getParameter(id));
    jassert(choiceParam != nullptr);

    for (int i = 0; i < choiceParam->choices.size(); ++i)
        combo->addItem(choiceParam->choices[i], i + 1);

    addAndMakeVisible(*combo);

    auto label = std::make_unique<juce::Label>();
    label->setText(choiceParam->getName(128), juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);

    auto attachment = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.apvts, id, *combo);

    comboBoxes.push_back(std::move(combo));
    comboBoxAttachments.push_back(std::move(attachment));
    comboBoxLabels.push_back(std::move(label));
}