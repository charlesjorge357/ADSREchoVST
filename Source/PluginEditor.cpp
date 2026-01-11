/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ADSREchoAudioProcessorEditor::ADSREchoAudioProcessorEditor (ADSREchoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    startTimerHz(30);
    
    addAndMakeVisible(moduleViewport);
    moduleViewport.setViewedComponent(&moduleContainer, false);
    moduleViewport.setScrollBarsShown(true, false);

    /*
    // MASTER MIX
    addAndMakeVisible(masterMixSlider);
    masterMixSlider.setSliderStyle(juce::Slider::Rotary);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "MasterMix", masterMixSlider);

    addAndMakeVisible(gainSlider);
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "Gain", gainSlider);

    // ALGORITHMIC
    addAndMakeVisible(algoToggle);
    algoToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "algoEnabled", algoToggle);

    addAndMakeVisible(algoWetDrySlider);
    algoWetDrySlider.setSliderStyle(juce::Slider::Rotary);
    algoWetDrySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    algoWetDryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "ReverbMix", algoWetDrySlider);

    // CONVOLUTION
    addAndMakeVisible(convToggle);
    convToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "convEnabled", convToggle);

    addAndMakeVisible(convWetDrySlider);
    convWetDrySlider.setSliderStyle(juce::Slider::Rotary);
    convWetDrySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    convWetDryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "ConvMix", convWetDrySlider);

    // Setup visibility listener
    algoToggle.onClick = [this]
        {
            algoWetDrySlider.setVisible(algoToggle.getToggleState());
        };
    convToggle.onClick = [this]
        {
            convWetDrySlider.setVisible(convToggle.getToggleState());
        };

    // Initialize visibility based on parameter state
    algoWetDrySlider.setVisible(audioProcessor.apvts.getRawParameterValue("algoEnabled")->load() > 0.5f);
    convWetDrySlider.setVisible(audioProcessor.apvts.getRawParameterValue("convEnabled")->load() > 0.5f);
    */

    addAndMakeVisible(addButton);

    addButton.onClick = [this]
    {
        audioProcessor.requestAddModule(ModuleType::Delay);
        attemptedChange = true;
    };

    setSize(600, 400);
}

ADSREchoAudioProcessorEditor::~ADSREchoAudioProcessorEditor()
{
}

//==============================================================================
void ADSREchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    //// (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //g.setColour (juce::Colours::white);
    //g.setFont (juce::FontOptions (15.0f));
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void ADSREchoAudioProcessorEditor::resized()
{
    //auto area = getLocalBounds().reduced(10);

    //auto top = area.removeFromTop(80);
    //masterMixSlider.setBounds(top.removeFromLeft(120));
    //gainSlider.setBounds(top.removeFromLeft(120));

    //auto bottom = area.removeFromTop(100);
    //algoToggle.setBounds(bottom.removeFromLeft(120));
    //algoWetDrySlider.setBounds(bottom.removeFromLeft(120));

    //convToggle.setBounds(bottom.removeFromLeft(120));
    //convWetDrySlider.setBounds(bottom.removeFromLeft(120));

    auto area = getLocalBounds().reduced(10);

    addButton.setBounds(area.removeFromTop(30));

    moduleViewport.setBounds(area);

    constexpr int slotHeight = 120;
    int y = 0;

    for (auto* editor : moduleEditors)
    {
        editor->setBounds(0, y, moduleViewport.getWidth(), slotHeight);
        y += slotHeight + 6;
    }

    moduleContainer.setSize(moduleViewport.getWidth(), y);
}

void ADSREchoAudioProcessorEditor::timerCallback()
{
    //if (moduleEditors.size() != processor.getNumModules())
    //    rebuildModuleEditors();
    if (attemptedChange) {
        rebuildModuleEditors();
        attemptedChange = false;
    }
}

void ADSREchoAudioProcessorEditor::rebuildModuleEditors()
{
    moduleEditors.clear();

    for (int i = 0; i < audioProcessor.getNumSlots(); i++)
    {
        if (audioProcessor.slotIsEmpty(i)) { continue; }
        
        auto info = audioProcessor.getSlotInfo(i);

        auto* editor = new ModuleSlotEditor(
            i,
            info,
            audioProcessor,
            audioProcessor.apvts
        );

        moduleEditors.add(editor);
        moduleContainer.addAndMakeVisible(editor);
    }

    resized();


    //for (int i = 0; i < testNumModules; ++i)
    //{
    //    SlotInfo info = { "testSlotID", "testSlottype" };

    //    auto* editor = new ModuleSlotEditor(
    //        i,
    //        info,
    //        audioProcessor,
    //        audioProcessor.apvts
    //    );

    //    moduleEditors.add(editor);
    //    moduleContainer.addAndMakeVisible(editor);
    //}

    //resized();
}
