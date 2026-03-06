/*
  ==============================================================================
    ModuleSlotEditor.cpp - WITH IR COMBOBOX
  ==============================================================================
*/

#include "ModuleSlotEditor.h"
#include "DelayPanel.h"
#include "ReverbPanel.h"
#include "ConvolutionPanel.h"

ModuleSlotEditor::ModuleSlotEditor(
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
}

void ModuleSlotEditor::buildEditor(const SlotInfo& info)
{
    if (info.moduleType == "Delay")
    {
        delayPanel = std::make_unique<DelayPanel>();
        delayPanel->attachToAPVTS(processor.apvts, slotID);
        addAndMakeVisible(*delayPanel);
    }
    else if (info.moduleType == "Reverb")
    {
        reverbPanel = std::make_unique<ReverbPanel>();
        reverbPanel->attachToAPVTS(processor.apvts, slotID);
        addAndMakeVisible(*reverbPanel);
    }
    else if (info.moduleType == "Convolution")
    {
        convolutionPanel = std::make_unique<ConvolutionPanel>();
        convolutionPanel->attachToAPVTS(processor.apvts, slotID,
                                        processor, chainIndex, slotIndex);
        addAndMakeVisible(*convolutionPanel);
    }
}

void ModuleSlotEditor::layoutEditor(juce::Rectangle<int>& r)
{
    if (delayPanel)            delayPanel->setBounds(r);
    else if (reverbPanel)      reverbPanel->setBounds(r);
    else if (convolutionPanel) convolutionPanel->setBounds(r);
}