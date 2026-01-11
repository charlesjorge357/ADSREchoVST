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

    //Module Enabled
    addAndMakeVisible(enableToggle);
    enableToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, slotID + ".enabled", enableToggle);

    //Module Mix Slider
    mixSlider.setSliderStyle(juce::Slider::Rotary);
    mixSlider.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 50, 18);
    addAndMakeVisible(mixSlider);

    mixAttachment =
        std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts,
            slotID + ".mix",
            mixSlider
        );

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
            //processor.requestRemoveModule(slotIndex);
        };
}

void ModuleSlotEditor::addSliderForParameter(juce::String id)
{
    auto slider = std::make_unique<juce::Slider>();
    
    slider->setSliderStyle(juce::Slider::Rotary);
    slider->setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 50, 18);

    addAndMakeVisible(*slider);

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment =
        std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts,
            id,
            *slider
        );
    sliders.push_back(std::move(slider));
    sliderAttachments.push_back(std::move(sliderAttachment));
}

void ModuleSlotEditor::resized()
{
    auto r = getLocalBounds().reduced(6);

    auto titleArea = r.removeFromTop(20);
    enableToggle.setBounds(titleArea.removeFromLeft(25));
    title.setBounds(titleArea);

    mixSlider.setBounds(r.removeFromLeft(80));
    for (auto& slider : sliders)
    {
        slider->setBounds(r.removeFromLeft(80));
    }
    removeButton.setBounds(r.removeFromRight(30));
}