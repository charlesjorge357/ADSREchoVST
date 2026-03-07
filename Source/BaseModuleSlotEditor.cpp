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

void BaseModuleSlotEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Column background
    g.setColour(juce::Colour(0xff222528));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Subtle border
    g.setColour(juce::Colour(0xff3A3E45));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

void BaseModuleSlotEditor::resized()
{
    auto r = getLocalBounds().reduced(4);

    // Control row: enable toggle | type selector | remove button
    auto controlRow = r.removeFromTop(24);
    enableToggle.setBounds(controlRow.removeFromLeft(25));
    removeButton.setBounds(controlRow.removeFromRight(30));
    typeSelector.setBounds(controlRow);

    r.removeFromTop(4);

    // Remaining area for panel content
    layoutEditor(r);
}