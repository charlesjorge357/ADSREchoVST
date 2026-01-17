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
    
    // MASTER MIX
    addAndMakeVisible(masterMixSlider);
    addAndMakeVisible(masterMixLabel);
    masterMixSlider.setSliderStyle(juce::Slider::Rotary);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "MasterMix", masterMixSlider);
    masterMixLabel.setText("Master Mix", juce::dontSendNotification);
    masterMixLabel.setJustificationType(juce::Justification::horizontallyCentred);

    //GAIN
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(gainLabel);
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "Gain", gainSlider);
    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::horizontallyCentred);


    // Add slot button
    addAndMakeVisible(addButton);
    addButton.onClick = [this]
    {
        audioProcessor.addModule(ModuleType::Delay);
        attemptedChange = true;
    };

    // Creates module viewport for the module container, giving scrollbar if enough modules are added    
    addAndMakeVisible(moduleViewport);
    moduleViewport.setViewedComponent(&moduleContainer, false);
    moduleViewport.setScrollBarsShown(true, false);

    setSize(800, 600);

    rebuildModuleEditors();
}

ADSREchoAudioProcessorEditor::~ADSREchoAudioProcessorEditor()
{
    // Stop the timer when the editor is destroyed
    stopTimer();
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
    auto area = getLocalBounds().reduced(10);

    // Arrange the layout of the master controls at the top
    auto top = area.removeFromTop(110);
    auto masterMixArea = top.removeFromLeft(120);
    auto gainArea = top.removeFromLeft(120);
    masterMixSlider.setBounds(masterMixArea.removeFromTop(80));
    masterMixLabel.setBounds(masterMixArea.removeFromTop(20));
    gainSlider.setBounds(gainArea.removeFromTop(80));
    gainLabel.setBounds(gainArea.removeFromTop(20));

    addButton.setBounds(area.removeFromTop(30));

    // Sets the layout of each Module Editor, created down sequentially in the module viewport.
    moduleViewport.setBounds(area);

    constexpr int slotHeight = 150;
    int y = 0;

    for (auto* editor : moduleEditors)
    {
        editor->setBounds(0, y, moduleViewport.getWidth(), slotHeight);
        y += slotHeight + 6;
    }

    moduleContainer.setSize(moduleViewport.getWidth(), y);
}

// On a constant timer, checks if the ui needs to be rebuild, then calls for a rebuild asynchronously
void ADSREchoAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.uiNeedsRebuild.exchange(false, std::memory_order_acquire))
        triggerAsyncUpdate();
}

void ADSREchoAudioProcessorEditor::handleAsyncUpdate()
{
    rebuildModuleEditors();
}

// Rebuilds the module editor list, based on the current module slot list
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
}


