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
    juce::LookAndFeel::setDefaultLookAndFeel(&customLNF);

    startTimerHz(30);
    currentlyDisplayedChain = 0;

    // Per-chain master panels
    auto* parallelParam = audioProcessor.apvts.getRawParameterValue("parallelEnabled");
    bool parallelEnabled = parallelParam->load() > 0.5f;

    for (int chain = 0; chain < numChains; ++chain)
    {
        masterPanels[chain] = std::make_unique<MasterPanel>(chain);
        masterPanels[chain]->attachToAPVTS(audioProcessor.apvts);
        addAndMakeVisible(masterPanels[chain].get());

        if (chain > 0 && !parallelEnabled)
            masterPanels[chain]->setVisible(false);
    }

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
            masterPanels[1]->setVisible(enabled);
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
    moduleViewport.setScrollBarsShown(false, true);

    setSize(900, 700);
    rebuildModuleEditors();
}

ADSREchoAudioProcessorEditor::~ADSREchoAudioProcessorEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void ADSREchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Base gradient — dark steel
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff2A2D32), 0.0f, 0.0f,
        juce::Colour(0xff1A1D22), 0.0f, bounds.getHeight(),
        false));
    g.fillRect(bounds);

    // Brushed-metal horizontal lines
    juce::Random rng(42);
    g.setColour(juce::Colour(0x08FFFFFF));
    for (float y = 0; y < bounds.getHeight(); y += 2.0f)
    {
        float alpha = 0.03f + rng.nextFloat() * 0.04f;
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
    }
}

void ADSREchoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto top = area.removeFromTop(110);

    // Master panels per chain
    for (int chain = 0; chain < numChains; ++chain)
        masterPanels[chain]->setBounds(top.removeFromLeft(160));

    chainSelector.setBounds(top.removeFromLeft(100));
    parallelEnableToggle.setBounds(top.removeFromLeft(30));

    addButton.setBounds(area.removeFromRight(40).removeFromTop(40));

    // Modules on the chain are displayed as side-by-side columns
    moduleViewport.setBounds(area);

    constexpr int columnWidth = 245;
    constexpr int columnSpacing = 6;
    int x = 0;
    int columnHeight = moduleViewport.getHeight();

    for (auto& editor : moduleEditors)
    {
        editor->setBounds(x, 0, columnWidth, columnHeight);
        x += columnWidth + columnSpacing;
    }

    moduleContainer.setSize(x, columnHeight);
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

        moduleContainer.addAndMakeVisible(editor.get());

        moduleEditors.push_back(std::move(editor));
    }

    resized();
}
