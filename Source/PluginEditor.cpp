/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ADSREchoAudioProcessorEditor::ADSREchoAudioProcessorEditor (ADSREchoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), presetManager (p)
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

    // Preset name editor
    presetNameEditor.setTextToShowWhenEmpty("Preset name...", juce::Colours::grey);
    presetNameEditor.setMultiLine(false);
    addAndMakeVisible(presetNameEditor);

    // Save button
    savePresetButton.onClick = [this]
        {
            auto name = presetNameEditor.getText().trim();
            if (name.isEmpty()) return;
            if (presetManager.savePreset(name))
            {
                refreshPresetComboBox();
                // Select the newly saved preset in the combo box
                auto names = presetManager.getPresetNames();
                int idx = names.indexOf(name);
                if (idx >= 0)
                    presetComboBox.setSelectedItemIndex(idx, juce::dontSendNotification);
            }
        };
    addAndMakeVisible(savePresetButton);

    // Preset combo box
    presetComboBox.setTextWhenNothingSelected("-- Load Preset --");
    presetComboBox.onChange = [this]
        {
            int idx = presetComboBox.getSelectedItemIndex();
            if (idx < 0) return;
            auto files = presetManager.getPresetFiles();
            if (juce::isPositiveAndBelow(idx, files.size()))
            {
                presetManager.loadPreset(files[idx]);
                presetNameEditor.setText(files[idx].getFileNameWithoutExtension(),
                                         juce::dontSendNotification);
            }
        };
    addAndMakeVisible(presetComboBox);
    refreshPresetComboBox();

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

    setSize(950, 800);
    rebuildModuleEditors();

    // Sync preset name on construction (e.g. after FL Studio window mode switch recreates the editor)
    const auto& name = audioProcessor.currentPresetName;
    if (name.isNotEmpty())
    {
        auto names = presetManager.getPresetNames();
        int idx = names.indexOf(name);
        presetComboBox.setSelectedId(idx >= 0 ? idx + 1 : 0, juce::dontSendNotification);
        presetNameEditor.setText(name, juce::dontSendNotification);
    }
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
    int w = static_cast<int>(bounds.getWidth());
    int h = static_cast<int>(bounds.getHeight());

    // Base fill — dark grey
    g.fillAll(juce::Colour(0xff2A2A2A));

    // Carbon-fiber weave pattern
    // Alternating light/dark squares in a diagonal twill pattern
    constexpr int cellSize = 6;
    juce::Random rng(42);

    for (int row = 0; row < h; row += cellSize)
    {
        for (int col = 0; col < w; col += cellSize)
        {
            // Diagonal twill: checkerboard offset every other row
            bool dark = ((col / cellSize) + (row / cellSize)) % 2 == 0;

            float base = dark ? 0.14f : 0.19f;
            float noise = rng.nextFloat() * 0.02f;
            float lum = base + noise;

            g.setColour(juce::Colour::fromFloatRGBA(lum, lum, lum, 1.0f));
            g.fillRect(col, row, cellSize, cellSize);
        }
    }

    // Fine grid lines for weave definition
    g.setColour(juce::Colour(0x18000000));
    for (int row = 0; row < h; row += cellSize)
        g.drawHorizontalLine(row, 0.0f, (float)w);
    for (int col = 0; col < w; col += cellSize)
        g.drawVerticalLine(col, 0.0f, (float)h);

    // Subtle top-to-bottom gradient overlay for depth
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x15FFFFFF), 0.0f, 0.0f,
        juce::Colour(0x10000000), 0.0f, bounds.getHeight(),
        false));
    g.fillRect(bounds);
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

    top.removeFromLeft(16); // spacer
    presetNameEditor.setBounds(top.removeFromLeft(150).withSizeKeepingCentre(150, 26));
    savePresetButton.setBounds(top.removeFromLeft(60).withSizeKeepingCentre(56, 26));
    top.removeFromLeft(8); // spacer
    presetComboBox.setBounds(top.removeFromLeft(200).withSizeKeepingCentre(200, 26));

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

    // Sync preset combobox to the restored preset name (mirrors Serum behaviour)
    refreshPresetComboBox();

    const auto& name = audioProcessor.currentPresetName;
    if (name.isNotEmpty())
    {
        auto names = presetManager.getPresetNames();
        int idx = names.indexOf(name);
        presetComboBox.setSelectedId(idx >= 0 ? idx + 1 : 0, juce::dontSendNotification);
        presetNameEditor.setText(name, juce::dontSendNotification);
    }
    else
    {
        presetComboBox.setSelectedId(0, juce::dontSendNotification);
        presetNameEditor.clear();
    }
}

void ADSREchoAudioProcessorEditor::ModuleContainer::paint(juce::Graphics& g)
{
    if (dropInsertionIndex < 0)
        return;

    constexpr int cw = 245, cs = 6;
    int barX = dropInsertionIndex * (cw + cs) - 2;
    barX = juce::jmax(0, barX);

    g.setColour(juce::Colour(0xff4A9EFF));
    g.fillRect(barX, 4, 3, getHeight() - 8);
}

int ADSREchoAudioProcessorEditor::getInsertionIndex(int screenX) const
{
    constexpr int cw = 245, cs = 6;
    auto local = moduleContainer.getLocalPoint(nullptr, juce::Point<int>(screenX, 0));
    int n = (int)moduleEditors.size();

    for (int i = 0; i < n; ++i)
    {
        int center = i * (cw + cs) + cw / 2;
        if (local.x < center)
            return i;
    }

    return n;
}

void ADSREchoAudioProcessorEditor::refreshPresetComboBox()
{
    presetComboBox.clear(juce::dontSendNotification);
    auto names = presetManager.getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetComboBox.addItem(names[i], i + 1);
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

        int editorIdx = (int)moduleEditors.size();
        int capturedSlot = i;

        editor->onDrag = [this, editorIdx](int /*slot*/, int screenX)
        {
            dragSourceIndex = editorIdx;
            int insert = getInsertionIndex(screenX);
            if (insert != dropInsertionIndex)
            {
                dropInsertionIndex = insert;
                moduleContainer.dropInsertionIndex = insert;
                moduleContainer.repaint();
            }
        };

        editor->onDrop = [this, editorIdx, capturedSlot](int /*slot*/, int screenX)
        {
            int insertIdx = getInsertionIndex(screenX);
            dragSourceIndex = -1;
            dropInsertionIndex = -1;
            moduleContainer.dropInsertionIndex = -1;
            moduleContainer.repaint();

            int to;
            if (insertIdx <= editorIdx)
                to = insertIdx;
            else if (insertIdx > editorIdx + 1)
                to = insertIdx - 1;
            else
                return; // same position

            if (to != editorIdx)
                audioProcessor.requestSlotMove(currentlyDisplayedChain, capturedSlot, to);
        };

        moduleEditors.push_back(std::move(editor));
    }

    resized();
}
