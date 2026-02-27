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

    // IR Selectors

    std::vector<std::unique_ptr<juce::ComboBox>> irSelectors;
    std::vector<std::unique_ptr<juce::Label>> irSelectorLabels;

    juce::TextButton browseIRButton{ "Browse" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    bool hasIRSelector = false;

    // Base overrides

    void buildEditor(const SlotInfo& info) override;

    void layoutEditor(juce::Rectangle<int>& r) override;

    // Helpers

    void addSliderForParameter(juce::String id);

    void addToggleForParameter(juce::String id);

    void addChoiceForParameter(juce::String id);

    void addIRSelectorForParameter(juce::String id);
};