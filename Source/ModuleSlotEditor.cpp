/*
  ==============================================================================

    ModuleSlotEditor.cpp
    UI Component Class for Effect Module Slots

  ==============================================================================
*/

#include "ModuleSlotEditor.h"

ModuleSlotEditor::ModuleSlotEditor(
    int index,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& apvts)
    : slotIndex(index),
    slotID(info.slotID),
    processor(p)
{
    //Module Title
    title.setText(info.moduleType, juce::dontSendNotification);
    addAndMakeVisible(title);

    //Module Type Selector
    addAndMakeVisible(typeSelector);
    typeSelector.addItem("Delay", 1);
    typeSelector.addItem("Datorro Hall", 2);
    typeSelector.addItem("Hybrid Plate", 3);
    if (info.moduleType == "Delay")
    {
        typeSelector.setSelectedId(1, juce::dontSendNotification);
    }
    else if (info.moduleType == "Datorro Hall")
    {
        typeSelector.setSelectedId(2, juce::dontSendNotification);
    }
    else if (info.moduleType == "Hybrid Plate")
    {
        typeSelector.setSelectedId(3, juce::dontSendNotification);
    }

    typeSelector.onChange = [this] 
    { 
        processor.changeModuleType(slotIndex, typeSelector.getSelectedId());
    };

    //Module Enabled
    addAndMakeVisible(enableToggle);
    enableToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, slotID + ".enabled", enableToggle);

    //Module Mix Slider
    //mixSlider.setSliderStyle(juce::Slider::Rotary);
    //mixSlider.setTextBoxStyle(
    //    juce::Slider::TextBoxBelow, false, 50, 18);
    //addAndMakeVisible(mixSlider);

    //mixAttachment =
    //    std::make_unique<
    //    juce::AudioProcessorValueTreeState::SliderAttachment>(
    //        apvts,
    //        slotID + ".mix",
    //        mixSlider
    //    );

    //Module Control Sliders
    auto usedParams = info.usedParameters;
    for (const auto& suffix : usedParams)
    {
        auto id = slotID + "." + suffix;
        addSliderForParameter(id);
    }

    //Module Remove Button
    addAndMakeVisible(removeButton);
    removeButton.onClick = [this]
        {
            processor.requestRemoveModule(slotIndex);
        };
}

void ModuleSlotEditor::addSliderForParameter(juce::String id)
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

void ModuleSlotEditor::resized()
{
    auto r = getLocalBounds().reduced(6);

    auto titleArea = r.removeFromTop(20);
    enableToggle.setBounds(titleArea.removeFromLeft(25));
    title.setBounds(titleArea.removeFromLeft(80));
    typeSelector.setBounds(titleArea);

    //mixSlider.setBounds(r.removeFromLeft(80));
    for (int i = 0; i < sliders.size(); i++)
    {
        auto a = r.removeFromLeft(80);
        sliderLabels[i]->setBounds(a.removeFromBottom(30));
        sliders[i]->setBounds(a);
    }
    removeButton.setBounds(r.removeFromRight(30));
}