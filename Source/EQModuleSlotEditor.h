/*
  ==============================================================================

    EQModuleSlotEditor.h
    Created: 2 Mar 2026 2:16:12pm
    Author:  Crabnebuelar

  ==============================================================================
*/

#pragma once

#include "BaseModuleSlotEditor.h"
#include "EQModule.h"
#include "EQDisplayComponent.h"
#include "EQPanel.h"

class EQModuleSlotEditor : public BaseModuleSlotEditor, private juce::Timer
{
public:

    EQModuleSlotEditor(
        int cIndex,
        int sIndex,
        const SlotInfo& info,
        ADSREchoAudioProcessor& processor,
        juce::AudioProcessorValueTreeState& apvts);

private:
    // Base overrides

    void buildEditor(const SlotInfo& info) override;

    void layoutEditor(juce::Rectangle<int>& r) override;

    EQModule* mod;
    std::unique_ptr<EQDisplayComponent> display;
    std::unique_ptr<EQPanel> panel;

    void timerCallback() override;

};