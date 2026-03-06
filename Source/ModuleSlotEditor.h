/*
  ==============================================================================
    ModuleSlotEditor.h - WITH IR COMBOBOX
  ==============================================================================
*/

#pragma once

#include "BaseModuleSlotEditor.h"
#include "ConvolutionModule.h"

class ModuleSlotEditor : public BaseModuleSlotEditor
{
public:

    ModuleSlotEditor(
        int cIndex,
        int sIndex,
        const SlotInfo& info,
        ADSREchoAudioProcessor& processor,
        juce::AudioProcessorValueTreeState& apvts);

private:
    std::unique_ptr<DelayPanel>       delayPanel;
    std::unique_ptr<ReverbPanel>      reverbPanel;
    std::unique_ptr<ConvolutionPanel> convolutionPanel;

    void buildEditor(const SlotInfo& info) override;
    void layoutEditor(juce::Rectangle<int>& r) override;
};