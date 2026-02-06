/*
  ==============================================================================
    ModuleSlotEditor.cpp - WITH IR COMBOBOX
  ==============================================================================
*/

#include "ModuleSlotEditor.h"

ModuleSlotEditor::ModuleSlotEditor(
    int cIndex, int sIndex,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& apvts)
    : chainIndex(cIndex), slotIndex(sIndex),
    slotID(info.slotID),
    processor(p)
{
    //Module Title
    title.setText(info.moduleType, juce::dontSendNotification);
    addAndMakeVisible(title);

    //Module Type Selector
    addAndMakeVisible(typeSelector);
    typeSelector.addItem("Delay", 1);
    typeSelector.addItem("Reverb", 2);
    typeSelector.addItem("Convolution", 3);

    if (info.moduleType == "Delay")
    {
        typeSelector.setSelectedId(1, juce::dontSendNotification);
    }
    else if (info.moduleType == "Reverb")
    {
        typeSelector.setSelectedId(2, juce::dontSendNotification);
    }
    else if (info.moduleType == "Convolution")
    {
        typeSelector.setSelectedId(3, juce::dontSendNotification);
    }

    typeSelector.onChange = [this] 
    { 
        processor.changeModuleType(chainIndex, slotIndex, static_cast<ModuleType>(typeSelector.getSelectedId()));
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
        auto* param = processor.apvts.getParameter(id);
        
        // Special handling for IR index - use ComboBox instead of slider
        if (suffix == "convIrIndex")
        {
            addIRSelectorForParameter(id);
        }
        else if (dynamic_cast<juce::AudioParameterBool*>(param))
        {
            addToggleForParameter(id);
        }
        else if (dynamic_cast<juce::AudioParameterChoice*>(param))
        {
            addChoiceForParameter(id);
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
            processor.removeModule(chainIndex, slotIndex);
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

void ModuleSlotEditor::addToggleForParameter(const juce::String id)
{
    auto toggle = std::make_unique<juce::ToggleButton>();

    addAndMakeVisible(*toggle);

    auto label = std::make_unique<juce::Label>();
    label->setText(processor.apvts.getParameter(id)->getName(128),
        juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);

    auto attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, id, *toggle);

    toggles.push_back(std::move(toggle));
    toggleAttachments.push_back(std::move(attachment));
    toggleLabels.push_back(std::move(label));
}

void ModuleSlotEditor::addChoiceForParameter(const juce::String id)
{
    auto combo = std::make_unique<juce::ComboBox>();

    auto* choiceParam =
        dynamic_cast<juce::AudioParameterChoice*>(
            processor.apvts.getParameter(id));

    jassert(choiceParam != nullptr);

    for (int i = 0; i < choiceParam->choices.size(); ++i)
        combo->addItem(choiceParam->choices[i], i + 1);

    addAndMakeVisible(*combo);

    auto label = std::make_unique<juce::Label>();
    label->setText(choiceParam->getName(128), juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);

    auto attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.apvts, id, *combo);

    comboBoxes.push_back(std::move(combo));
    comboBoxAttachments.push_back(std::move(attachment));
    comboBoxLabels.push_back(std::move(label));
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

    r.removeFromTop(5);
    r.removeFromRight(5);

    removeButton.setBounds(r.removeFromRight(30));

    // Layout IR selectors
    for (int i = 0; i < irSelectors.size(); i++)
    {
        auto a = r.removeFromLeft(70);  // Wider for ComboBox
        irSelectorLabels[i]->setBounds(a.removeFromBottom(30));
        irSelectors[i]->setBounds(a.removeFromTop(25));
    }

    // Layout Other Components
    for (int i = 0; i < sliders.size(); i++)
    {
        auto a = r.removeFromLeft(70);
        sliderLabels[i]->setBounds(a.removeFromBottom(30));
        sliders[i]->setBounds(a);
    }
    
    for (int i = 0; i < comboBoxes.size(); i++)
    {
        auto a = r.removeFromLeft(70);
        comboBoxLabels[i]->setBounds(a.removeFromBottom(30));
        comboBoxes[i]->setBounds(a.removeFromTop(25));
    }

    for (int i = 0; i < toggles.size(); i++)
    {
        auto a = r.removeFromLeft(70);
        toggleLabels[i]->setBounds(a.removeFromBottom(30));
        toggles[i]->setBounds(a.removeFromTop(25));
    }


}