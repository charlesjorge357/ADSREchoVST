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
    auto r = getLocalBounds().reduced(4);

    // Title at the top of the column
    title.setBounds(r.removeFromTop(22));
    r.removeFromTop(2);

    // Control row: enable toggle | type selector | remove button
    auto controlRow = r.removeFromTop(24);
    enableToggle.setBounds(controlRow.removeFromLeft(25));
    removeButton.setBounds(controlRow.removeFromRight(30));
    typeSelector.setBounds(controlRow);

    r.removeFromTop(4);

    // Remaining area for panel content
    layoutEditor(r);
}