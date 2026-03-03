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

    // Sliders

    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> sliderLabels;

    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>>
        sliderAttachments;

    // ComboBoxes

    std::vector<std::unique_ptr<juce::ComboBox>> comboBoxes;
    std::vector<std::unique_ptr<juce::Label>> comboBoxLabels;

    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>>
        comboBoxAttachments;

    // Toggles

    std::vector<std::unique_ptr<juce::Button>> toggles;
    std::vector<std::unique_ptr<juce::Label>> toggleLabels;

    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>>
        toggleAttachments;


    // Base overrides

    void buildEditor(const SlotInfo& info) override;

    void layoutEditor(juce::Rectangle<int>& r) override;

    // Helpers

    void addSliderForParameter(juce::String id);

    void addToggleForParameter(juce::String id);

    void addChoiceForParameter(juce::String id);

    EQModule* mod;
    std::unique_ptr<EQDisplayComponent> display;

    void timerCallback() override;

};