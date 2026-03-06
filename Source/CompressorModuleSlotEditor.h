/*
  ==============================================================================

    CompressorModuleSlotEditor.h
    Slot editor for the compressor module.  Mirrors EQModuleSlotEditor:
    - Builds rotary sliders from APVTS parameters
    - Owns a CompressorDisplayComponent
    - Timer polls CompressorModule::meterReady and pushes values to the display

  ==============================================================================
*/

#pragma once

#include "BaseModuleSlotEditor.h"
#include "CompressorModule.h"
#include "CompressorDisplayComponent.h"
#include "CompressorPanel.h"

class CompressorModuleSlotEditor : public BaseModuleSlotEditor,
                                   private juce::Timer
{
public:
    CompressorModuleSlotEditor(
        int cIndex,
        int sIndex,
        const SlotInfo& info,
        ADSREchoAudioProcessor& processor,
        juce::AudioProcessorValueTreeState& apvts);

private:

    // Base overrides
    void buildEditor(const SlotInfo& info) override;
    void layoutEditor(juce::Rectangle<int>& r) override;


    CompressorModule*                        mod     = nullptr;
    std::unique_ptr<CompressorDisplayComponent> display;
    std::unique_ptr<CompressorPanel> panel;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorModuleSlotEditor)
};