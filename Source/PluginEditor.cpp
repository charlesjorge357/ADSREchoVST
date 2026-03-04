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

    // Per-chain controls
    for (int chain = 0; chain < numChains; ++chain)
        setupChainControls(chain);

    // Chain selector
    addAndMakeVisible(chainSelector);
    for (int i = 0; i < numChains; ++i)
        chainSelector.addItem("Chain " + juce::String(i + 1), i + 1);

    chainSelector.setSelectedId(1, juce::dontSendNotification);
    chainSelector.onChange = [this]
        {
            currentlyDisplayedChain = chainSelector.getSelectedId() - 1;
            rebuildModuleEditors();
        };

    // Parallel toggle
    addAndMakeVisible(parallelEnableToggle);
    parallelEnableToggleAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.apvts, "parallelEnabled", parallelEnableToggle);

    parallelEnableToggle.onClick = [this]
        {
            bool enabled = parallelEnableToggle.getToggleState();

            masterMixSliders[1].setVisible(enabled);
            masterMixLabels[1].setVisible(enabled);
            gainSliders[1].setVisible(enabled);
            gainLabels[1].setVisible(enabled);
        };

    // Add module button
    addAndMakeVisible(addButton);
    addButton.onClick = [this]
        {
            audioProcessor.addModule(currentlyDisplayedChain, ModuleType::Delay);
            attemptedChange = true;
        };

    // Module viewport
    addAndMakeVisible(moduleViewport);
    moduleViewport.setViewedComponent(&moduleContainer, false);
    moduleViewport.setScrollBarsShown(true, false);

    setSize(900, 700);
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

}

void ADSREchoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto top = area.removeFromTop(110);

    // Gain and mix sliders per chain
    for (int chain = 0; chain < numChains; ++chain)
    {
        auto chainArea = top.removeFromLeft(240);

        auto mixArea = chainArea.removeFromLeft(120);
        auto gainArea = chainArea.removeFromLeft(120);

        masterMixSliders[chain].setBounds(mixArea.removeFromTop(80));
        masterMixLabels[chain].setBounds(mixArea.removeFromTop(20));

        gainSliders[chain].setBounds(gainArea.removeFromTop(80));
        gainLabels[chain].setBounds(gainArea.removeFromTop(20));
    }

    chainSelector.setBounds(top.removeFromLeft(100));
    parallelEnableToggle.setBounds(top.removeFromLeft(30));

    addButton.setBounds(area.removeFromTop(30));

    // Modules on the chain are added down sequentially
    moduleViewport.setBounds(area);

    constexpr int slotHeight = 200;
    int y = 0;

    for (auto& editor : moduleEditors)
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

// Setup for each chain mixer/gain slider
void ADSREchoAudioProcessorEditor::setupChainControls(int chainIndex)
{
    auto& mixSlider = masterMixSliders[chainIndex];
    auto& mixLabel = masterMixLabels[chainIndex];
    auto& gainSlider = gainSliders[chainIndex];
    auto& gainLabel = gainLabels[chainIndex];

    addAndMakeVisible(mixSlider);
    addAndMakeVisible(mixLabel);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(gainLabel);

    mixSlider.setSliderStyle(juce::Slider::Rotary);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

    gainSlider.setSliderStyle(juce::Slider::Rotary);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

    const juce::String chain = juce::String(chainIndex);

    masterMixAttachments[chainIndex] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "chain_" + chain + ".masterMix", mixSlider);

    gainAttachments[chainIndex] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "chain_" + chain + ".gain", gainSlider);

    mixLabel.setText("Master Mix (Chain " + juce::String(chainIndex + 1) + ")",
        juce::dontSendNotification);
    gainLabel.setText("Gain (Chain " + juce::String(chainIndex + 1) + ")",
        juce::dontSendNotification);

    mixLabel.setJustificationType(juce::Justification::horizontallyCentred);
    gainLabel.setJustificationType(juce::Justification::horizontallyCentred);

    // Set Sliders invisible if 2nd chain not active
    auto* param = audioProcessor.apvts.getRawParameterValue("parallelEnabled");
    bool parallelMixersEnabled = param->load() > 0.5f;;

    if (chainIndex > 0 && !parallelMixersEnabled) {
        mixSlider.setVisible(false);
        mixLabel.setVisible(false);
        gainSlider.setVisible(false);
        gainLabel.setVisible(false);
    }
}


// Rebuilds the module editor list, based on the current module slot list
void ADSREchoAudioProcessorEditor::rebuildModuleEditors()
{
    moduleEditors.clear();

    moduleContainer.removeAllChildren();

    for (int i = 0; i < audioProcessor.getNumSlots(); ++i)
    {
        if (audioProcessor.slotIsEmpty(currentlyDisplayedChain, i))
            continue;

        auto info =
            audioProcessor.getSlotInfo(
                currentlyDisplayedChain, i);

        std::unique_ptr<BaseModuleSlotEditor> editor;

        if (info.moduleType == "EQ") {
            editor =
                std::make_unique<EQModuleSlotEditor>(
                    currentlyDisplayedChain,
                    i,
                    info,
                    audioProcessor,
                    audioProcessor.apvts
                );
        }
        else if (info.moduleType == "Compressor")
        {
            editor = 
                std::make_unique<CompressorModuleSlotEditor>(
                    currentlyDisplayedChain, 
                    i, 
                    info,
                    audioProcessor, 
                    audioProcessor.apvts
                );
        }
        else {
            editor =
                std::make_unique<ModuleSlotEditor>(
                    currentlyDisplayedChain,
                    i,
                    info,
                    audioProcessor,
                    audioProcessor.apvts
                );
        }

        //auto editor =
        //    std::make_unique<ModuleSlotEditor>(
        //        currentlyDisplayedChain,
        //        i,
        //        info,
        //        audioProcessor,
        //        audioProcessor.apvts
        //    );

        moduleContainer.addAndMakeVisible(editor.get());

        moduleEditors.push_back(std::move(editor));
    }

    resized();
}