/*
  ==============================================================================



  ==============================================================================
*/

#include "BaseModuleSlotEditor.h"

BaseModuleSlotEditor::BaseModuleSlotEditor(
    int cIndex,
    int sIndex,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& state)
    : chainIndex(cIndex),
    slotIndex(sIndex),
    slotID(info.slotID),
    processor(p),
    apvts(state)
{
    // Title

    title.setText(info.moduleType,
        juce::dontSendNotification);

    addAndMakeVisible(title);


    // Module selector

    addAndMakeVisible(typeSelector);

    typeSelector.addItem("Delay", 1);
    typeSelector.addItem("Reverb", 2);
    typeSelector.addItem("Convolution", 3);
    typeSelector.addItem("EQ", 4);
    typeSelector.addItem("Compressor", 5);


    if (info.moduleType == "Delay")
        typeSelector.setSelectedId(
            1, juce::dontSendNotification);

    else if (info.moduleType == "Reverb")
        typeSelector.setSelectedId(
            2, juce::dontSendNotification);

    else if (info.moduleType == "Convolution")
        typeSelector.setSelectedId(
            3, juce::dontSendNotification);

    else if (info.moduleType == "EQ")
        typeSelector.setSelectedId(
            4, juce::dontSendNotification);

    else if (info.moduleType == "Compressor")
        typeSelector.setSelectedId(
            5, juce::dontSendNotification);


    typeSelector.onChange = [this]
        {
            processor.changeModuleType(
                chainIndex,
                slotIndex,
                static_cast<ModuleType>(
                    typeSelector.getSelectedId()));
        };


    // Enable toggle

    addAndMakeVisible(enableToggle);

    enableToggleAttachment =
        std::make_unique<
        juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts,
            slotID + ".enabled",
            enableToggle);


    // Remove button

    addAndMakeVisible(removeButton);

    removeButton.onClick = [this]
        {
            processor.removeModule(
                chainIndex,
                slotIndex);
        };

}


void BaseModuleSlotEditor::resized()
{
    auto r = getLocalBounds().reduced(6);


    auto titleArea = r.removeFromTop(20);

    enableToggle.setBounds(
        titleArea.removeFromLeft(25));

    title.setBounds(
        titleArea.removeFromLeft(80));

    typeSelector.setBounds(titleArea);


    r.removeFromTop(5);


    removeButton.setBounds(
        r.removeFromRight(30));


    layoutEditor(r);
}