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
    
    currentlyDisplayedChain = 0;

    // MASTER MIX (Chain 0)
    addAndMakeVisible(masterMixSlider0);
    addAndMakeVisible(masterMixLabel0);
    masterMixSlider0.setSliderStyle(juce::Slider::Rotary);
    masterMixSlider0.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    masterMixAttachment0 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "chain_0.masterMix", masterMixSlider0);
    masterMixLabel0.setText("Master Mix (Chain 1)", juce::dontSendNotification);
    masterMixLabel0.setJustificationType(juce::Justification::horizontallyCentred);

    // GAIN (Chain 0)
    addAndMakeVisible(gainSlider0);
    addAndMakeVisible(gainLabel0);
    gainSlider0.setSliderStyle(juce::Slider::Rotary);
    gainSlider0.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    gainAttachment0 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "chain_0.gain", gainSlider0);
    gainLabel0.setText("Gain (Chain 1)", juce::dontSendNotification);
    gainLabel0.setJustificationType(juce::Justification::horizontallyCentred);

    addAndMakeVisible(masterMixSlider1);
    addAndMakeVisible(masterMixLabel1);
    masterMixSlider1.setSliderStyle(juce::Slider::Rotary);
    masterMixSlider1.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    masterMixAttachment1 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "chain_1.masterMix", masterMixSlider1);
    masterMixLabel1.setText("Master Mix (Chain 2)", juce::dontSendNotification);
    masterMixLabel1.setJustificationType(juce::Justification::horizontallyCentred);

    // GAIN (Chain 0)
    addAndMakeVisible(gainSlider1);
    addAndMakeVisible(gainLabel1);
    gainSlider1.setSliderStyle(juce::Slider::Rotary);
    gainSlider1.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    gainAttachment1 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "chain_1.gain", gainSlider1);
    gainLabel1.setText("Gain (Chain 2)", juce::dontSendNotification);
    gainLabel1.setJustificationType(juce::Justification::horizontallyCentred);

    // Chain Selector and Toggle
    addAndMakeVisible(chainSelector);
    chainSelector.addItem("Chain 1", 1);
    chainSelector.addItem("Chain 2", 2);

    chainSelector.setSelectedId(1, juce::dontSendNotification);

    chainSelector.onChange = [this]
        {
            currentlyDisplayedChain = chainSelector.getSelectedId() - 1;
            rebuildModuleEditors();
        };

    addAndMakeVisible(parallelEnableToggle);
    parallelEnableToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "parallelEnabled", parallelEnableToggle);

    // Add slot button
    addAndMakeVisible(addButton);
    addButton.onClick = [this]
    {
        audioProcessor.addModule(currentlyDisplayedChain, ModuleType::Delay);
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
    auto masterMixArea0 = top.removeFromLeft(120);
    auto gainArea0 = top.removeFromLeft(120);
    masterMixSlider0.setBounds(masterMixArea0.removeFromTop(80));
    masterMixLabel0.setBounds(masterMixArea0.removeFromTop(20));
    gainSlider0.setBounds(gainArea0.removeFromTop(80));
    gainLabel0.setBounds(gainArea0.removeFromTop(20));

    auto masterMixArea1 = top.removeFromLeft(120);
    auto gainArea1 = top.removeFromLeft(120);
    masterMixSlider1.setBounds(masterMixArea1.removeFromTop(80));
    masterMixLabel1.setBounds(masterMixArea1.removeFromTop(20));
    gainSlider1.setBounds(gainArea1.removeFromTop(80));
    gainLabel1.setBounds(gainArea1.removeFromTop(20));

    chainSelector.setBounds(top.removeFromLeft(90));

    parallelEnableToggle.setBounds(top.removeFromLeft(25));

    addButton.setBounds(area.removeFromTop(30));

    // Sets the layout of each Module Editor, created down sequentially in the module viewport.
    moduleViewport.setBounds(area);

    constexpr int slotHeight = 160;
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
        if (audioProcessor.slotIsEmpty(currentlyDisplayedChain, i)) { continue; }
        
        auto info = audioProcessor.getSlotInfo(currentlyDisplayedChain, i);

        auto* editor = new ModuleSlotEditor(
            currentlyDisplayedChain,
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


