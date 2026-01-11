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
    title.setText(info.moduleType, juce::dontSendNotification);
    addAndMakeVisible(title);

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

    addAndMakeVisible(removeButton);
    removeButton.onClick = [this]
        {
            //processor.requestRemoveModule(slotIndex);
        };
}

void ModuleSlotEditor::resized()
{
    auto r = getLocalBounds().reduced(6);

    title.setBounds(r.removeFromTop(20));
    mixSlider.setBounds(r.removeFromLeft(80));
    removeButton.setBounds(r.removeFromRight(24));
}