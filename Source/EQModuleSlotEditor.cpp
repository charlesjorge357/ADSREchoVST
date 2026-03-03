/*
  ==============================================================================
    EQModuleSlotEditor.cpp
  ==============================================================================
*/

#include "EQModuleSlotEditor.h"

EQModuleSlotEditor::EQModuleSlotEditor(
    int cIndex,
    int sIndex,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& state)

    : BaseModuleSlotEditor(
        cIndex,
        sIndex,
        info,
        p,
        state)
{
    buildEditor(info);
    startTimerHz(60);
}

void EQModuleSlotEditor::buildEditor(const SlotInfo& info)
{
    auto usedParams = info.usedParameters;

    for (const auto& suffix : usedParams)
    {
        auto id = slotID + "." + suffix;

        auto* param = processor.apvts.getParameter(id);

        if (dynamic_cast<
            juce::AudioParameterBool*>(param))
            addToggleForParameter(id);

        else if (dynamic_cast<
            juce::AudioParameterChoice*>(param))
            addChoiceForParameter(id);

        else
            addSliderForParameter(id);
    }

    mod = dynamic_cast<EQModule*>(processor.slots[chainIndex][slotIndex]->get());

    display = std::make_unique<EQDisplayComponent>(*mod);
    addAndMakeVisible(*display);
}

void EQModuleSlotEditor::addSliderForParameter(juce::String id)
{
    //Add Slider
    auto slider = std::make_unique<juce::Slider>();
    slider->setSliderStyle(juce::Slider::Rotary);
    slider->setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 50, 18);

    addAndMakeVisible(*slider);

    //Add Slider Label
    auto sliderLabel = std::make_unique<juce::Label>();
    sliderLabel->setText(processor.apvts.getParameter(id)->getName(128), juce::dontSendNotification);
    sliderLabel->setJustificationType(juce::Justification::centred);

    addAndMakeVisible(*sliderLabel);

    //Attach Slider to APVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment =
        std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts,
            id,
            *slider
        );

    sliders.push_back(std::move(slider));
    sliderAttachments.push_back(std::move(sliderAttachment));
    sliderLabels.push_back(std::move(sliderLabel));
}

void EQModuleSlotEditor::addToggleForParameter(const juce::String id)
{
    auto toggle = std::make_unique<juce::ToggleButton>();

    addAndMakeVisible(*toggle);

    auto label = std::make_unique<juce::Label>();
    label->setText(processor.apvts.getParameter(id)->getName(128),
        juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);

    auto attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, id, *toggle);

    toggles.push_back(std::move(toggle));
    toggleAttachments.push_back(std::move(attachment));
    toggleLabels.push_back(std::move(label));
}

void EQModuleSlotEditor::addChoiceForParameter(const juce::String id)
{
    auto combo = std::make_unique<juce::ComboBox>();

    auto* choiceParam =
        dynamic_cast<juce::AudioParameterChoice*>(
            processor.apvts.getParameter(id));

    jassert(choiceParam != nullptr);

    for (int i = 0; i < choiceParam->choices.size(); ++i)
        combo->addItem(choiceParam->choices[i], i + 1);

    addAndMakeVisible(*combo);

    auto label = std::make_unique<juce::Label>();
    label->setText(choiceParam->getName(128), juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);

    auto attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.apvts, id, *combo);

    comboBoxes.push_back(std::move(combo));
    comboBoxAttachments.push_back(std::move(attachment));
    comboBoxLabels.push_back(std::move(label));
}

void EQModuleSlotEditor::layoutEditor(juce::Rectangle<int>& r)
{
    auto displayArea = r.removeFromLeft(200);

    if (display)
        display->setBounds(displayArea.reduced(5));


    for (int i = 0;i < sliders.size();i++)
    {
        auto a = r.removeFromLeft(70);

        sliderLabels[i]->setBounds(
            a.removeFromBottom(30));

        sliders[i]->setBounds(a);
    }

    for (int i = 0;i < comboBoxes.size();i++)
    {
        auto a = r.removeFromLeft(70);

        comboBoxLabels[i]->setBounds(
            a.removeFromBottom(30));

        comboBoxes[i]->setBounds(
            a.removeFromTop(25));
    }

    for (int i = 0;i < toggles.size();i++)
    {
        auto a = r.removeFromLeft(70);

        toggleLabels[i]->setBounds(
            a.removeFromBottom(30));

        toggles[i]->setBounds(
            a.removeFromTop(25));
    }
}

void EQModuleSlotEditor::timerCallback()
{
    if (mod->fftReady)
    {
        display->pushSamples(mod->fftBuffer,
            mod->fftSize);

        mod->fftReady = false;
    }
}