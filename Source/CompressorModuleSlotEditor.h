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
    // Sliders
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>>  sliderLabels;
    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    // Toggles
    std::vector<std::unique_ptr<juce::Button>> toggles;
    std::vector<std::unique_ptr<juce::Label>>  toggleLabels;
    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAttachments;

    // ComboBoxes
    std::vector<std::unique_ptr<juce::ComboBox>> comboBoxes;
    std::vector<std::unique_ptr<juce::Label>>    comboBoxLabels;
    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboBoxAttachments;

    // Base overrides
    void buildEditor(const SlotInfo& info) override;
    void layoutEditor(juce::Rectangle<int>& r) override;

    // Helpers (identical to EQModuleSlotEditor)
    void addSliderForParameter(const juce::String& id);
    void addToggleForParameter(const juce::String& id);
    void addChoiceForParameter(const juce::String& id);

    CompressorModule*                        mod     = nullptr;
    std::unique_ptr<CompressorDisplayComponent> display;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorModuleSlotEditor)
};