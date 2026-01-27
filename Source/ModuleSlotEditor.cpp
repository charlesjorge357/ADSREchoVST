/*
  ==============================================================================
    ModuleSlotEditor.cpp - WITH IR COMBOBOX
  ==============================================================================
*/

#include "ModuleSlotEditor.h"

ModuleSlotEditor::ModuleSlotEditor(
    int index,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& apvts)
    : slotIndex(index),
    slotID(info.slotID),
    processor(p)
{
    //Module Title
    title.setText(info.moduleType, juce::dontSendNotification);
    addAndMakeVisible(title);

    //Module Type Selector
    addAndMakeVisible(typeSelector);
    typeSelector.addItem("Delay", 1);
    typeSelector.addItem("Datorro Hall", 2);
    typeSelector.addItem("Hybrid Plate", 3);
    typeSelector.addItem("Convolution", 4);

    if (info.moduleType == "Delay")
    {
        typeSelector.setSelectedId(1, juce::dontSendNotification);
    }
    else if (info.moduleType == "Datorro Hall")
    {
        typeSelector.setSelectedId(2, juce::dontSendNotification);
    }
    else if (info.moduleType == "Hybrid Plate")
    {
        typeSelector.setSelectedId(3, juce::dontSendNotification);
    }
    else if (info.moduleType == "Convolution")
    {
        typeSelector.setSelectedId(4, juce::dontSendNotification);
    }

    typeSelector.onChange = [this] 
    { 
        processor.changeModuleType(slotIndex, typeSelector.getSelectedId());
    };

    //Module Enabled
    addAndMakeVisible(enableToggle);
    enableToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, slotID + ".enabled", enableToggle);

    //Module Control Sliders (and ComboBoxes for special params)
    auto usedParams = info.usedParameters;
    for (const auto& suffix : usedParams)
    {
        auto id = slotID + "." + suffix;
        
        // Special handling for IR index - use ComboBox instead of slider
        if (suffix == "conv ir index")
        {
            addIRSelectorForParameter(id);
        }
        else
        {
            addSliderForParameter(id);
        }
    }

    //Module Remove Button
    addAndMakeVisible(removeButton);
    removeButton.onClick = [this]
        {
            processor.removeModule(slotIndex);
        };
}

void ModuleSlotEditor::addSliderForParameter(juce::String id)
{
    //Add Slider
    auto slider = std::make_unique<juce::Slider>();
    slider->setSliderStyle(juce::Slider::Rotary);
    slider->setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 50, 18);

    addAndMakeVisible(*slider);

    //Add Slider Label
    auto sliderLabel = std::make_unique<juce::Label>();
    sliderLabel->setText(processor.apvts.getParameter(id)->getName(128), juce::dontSendNotification);
    sliderLabel->setJustificationType(juce::Justification::centred);

    addAndMakeVisible(*sliderLabel);

    //Attach Slider to APVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment =
        std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts,
            id,
            *slider
        );

    sliders.push_back(std::move(slider));
    sliderAttachments.push_back(std::move(sliderAttachment));
    sliderLabels.push_back(std::move(sliderLabel));
}

void ModuleSlotEditor::addIRSelectorForParameter(juce::String id)
{
    // Add ComboBox for IR selection
    auto irSelector = std::make_unique<juce::ComboBox>();
    
    // Populate with IR names from IRBank
    auto irBank = processor.getIRBank();
    if (irBank)
    {
        for (int i = 0; i < irBank->getNumIRs(); ++i)
        {
            irSelector->addItem(irBank->getIRName(i), i + 1);  // ID starts at 1
        }
    }
    
    // Get current parameter value and set selection
    auto* param = processor.apvts.getRawParameterValue(id);
    if (param)
    {
        int currentIndex = (int)param->load();
        irSelector->setSelectedId(currentIndex + 1, juce::dontSendNotification);
    }
    
    // Update parameter when selection changes (same pattern as typeSelector)
    irSelector->onChange = [this, id, irSelectorPtr = irSelector.get()]
    {
        int selectedIndex = irSelectorPtr->getSelectedId() - 1;  // Convert back to 0-based
        auto* param = processor.apvts.getParameter(id);
        if (param)
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1((float)selectedIndex));
            param->endChangeGesture();
        }
    };
    
    addAndMakeVisible(*irSelector);
    
    // Add Label
    auto label = std::make_unique<juce::Label>();
    label->setText("IR", juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);
    
    // Store in vectors
    irSelectors.push_back(std::move(irSelector));
    irSelectorLabels.push_back(std::move(label));
}

void ModuleSlotEditor::resized()
{
    auto r = getLocalBounds().reduced(6);

    auto titleArea = r.removeFromTop(20);
    enableToggle.setBounds(titleArea.removeFromLeft(25));
    title.setBounds(titleArea.removeFromLeft(80));
    typeSelector.setBounds(titleArea);

    // Layout sliders
    for (int i = 0; i < sliders.size(); i++)
    {
        auto a = r.removeFromLeft(80);
        sliderLabels[i]->setBounds(a.removeFromBottom(30));
        sliders[i]->setBounds(a);
    }
    
    // Layout IR selectors
    for (int i = 0; i < irSelectors.size(); i++)
    {
        auto a = r.removeFromLeft(100);  // Wider for ComboBox
        irSelectorLabels[i]->setBounds(a.removeFromBottom(30));
        irSelectors[i]->setBounds(a.removeFromTop(25));
    }

    removeButton.setBounds(r.removeFromRight(30));
}