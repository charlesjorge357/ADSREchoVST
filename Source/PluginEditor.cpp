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

    //CustomLNF::setDefaultLookAndFeel(&customTypeFace);

    //addAndMakeVisible(viewport);
    //viewport.setViewedComponent(&container, false);
    //viewport.setScrollBarPosition(false,true);
    //
    //
    //reverbButton.setButtonText("Reverb");
    //addAndMakeVisible(reverbButton);

    //convolveButton.setButtonText("Convolve");
    //addAndMakeVisible(convolveButton);

    //delayButton.setButtonText("Delay");
    //addAndMakeVisible(delayButton);

    //equalizerButton.setButtonText("Equalizer");
    //addAndMakeVisible(equalizerButton);

    //compressorButton.setButtonText("Compressor");
    //addAndMakeVisible(compressorButton);


    //container.addAndMakeVisible(delayPanel);
    //container.addAndMakeVisible(convolvePanel);
    //container.addAndMakeVisible(revebPanel);
    //container.addAndMakeVisible(equalizerPanel);
    //container.addAndMakeVisible(compressorPanel);

    //masterMix.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    //masterMix.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    //masterMix.setRange(0.0, 100.0, 1.0); // 0-100%
    //masterMix.setValue(50.0); // Start at 50% mix
    //addAndMakeVisible(masterMix);


    //setSize (1250, 750);
    
}

ADSREchoAudioProcessorEditor::~ADSREchoAudioProcessorEditor()
{
    // Stop the timer when the editor is destroyed
    stopTimer();
}

//==============================================================================
void ADSREchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    customImage = juce::ImageCache::getFromMemory(BinaryData::JUCEBack_png, BinaryData::JUCEBack_pngSize);
    g.drawImageAt(customImage,0,0);

    //g.fillAll(juce::Colour(0xff1B1F23));



    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));

    auto bounds = getLocalBounds();
    auto sideBar = bounds.removeFromLeft(200).reduced(10).toFloat();

    sideBar = sideBar.removeFromTop(400);

    g.setColour(juce::Colour(0xff2B2E33));
    g.fillRoundedRectangle(sideBar, 10.0f);

    g.setColour(juce::Colour(0xff262B31));
    g.drawRoundedRectangle(sideBar,10.0f,1.5f);

    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
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
    //std::cout << ("Editor resized!");

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    //auto bounds = getLocalBounds();

    //customImage.setBoundsRelative(0.0f, 0.0f, 0.25f, 0.25f);


    //auto sidebarArea = bounds.removeFromLeft(200);
    //sidebarArea = sidebarArea.removeFromTop(350);
    
    //// Master Mix section at the bottom
    //auto masterMixArea = bounds.removeFromBottom(100);

    //auto buttonArea = sidebarArea.reduced(20,15);

    //int buttonHeight = 50;    

    //auto panelArea = bounds;
    //panelArea.reduce(15,15);

    //int panelWidth = (panelArea.getWidth()-20) / 4;
    //int panelspacing = 10;
    //masterMixArea.reduce(20, 10); // Add some padding
    
    // Center the knob horizontally
    //int knobSize = 80;
    //auto knobArea = masterMixArea.withSizeKeepingCentre(knobSize, knobSize);
    //masterMix.setBounds(knobArea);
    
    // Label above the knob
   /* auto labelArea = juce::Rectangle<int>(
        knobArea.getX() - 50, 
        knobArea.getY() - 25, 
        knobSize + 100, 
        20
    );*/
    //masterMixLabel.setBounds(labelArea);

    /*int spacings = 5;
    buttonArea.removeFromTop(50);
    reverbButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(spacings);
    convolveButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(spacings);
    delayButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(spacings);
    equalizerButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(spacings);
    compressorButton.setBounds(buttonArea.removeFromTop(buttonHeight));*/


    //viewport.setBounds(bounds.reduced(15,15));
    //const int panelWidth = 245;   // choose what looks good
    //const int panelSpacing = 10;

    //delayPanel.setBounds(panelArea);

    //auto dim = panelArea.removeFromLeft(panelWidth);

   // delayPanel.setBounds(dim);

    //td::cout << "delay panel dim" << dim.getWidth() << "x" << dim.getHeight();

    //int xPosition = 0;
    //int panelHeight = viewport.getHeight();

    //delayPanel.setBounds(xPosition, 0, panelWidth, panelHeight);
    //xPosition += panelWidth + panelSpacing;

    //convolvePanel.setBounds(xPosition, 0, panelWidth, panelHeight);
    //xPosition += panelWidth + panelSpacing;

    //revebPanel.setBounds(xPosition, 0, panelWidth, panelHeight);
    //xPosition += panelWidth + panelSpacing;

    //equalizerPanel.setBounds(xPosition, 0, panelWidth, panelHeight);
    //xPosition += panelWidth + panelSpacing;

    //compressorPanel.setBounds(xPosition,0,panelWidth,panelHeight);
    //xPosition += panelWidth + panelSpacing;

    

    
    //container.setSize(xPosition, panelHeight);



    /*panelArea.removeFromLeft(panelspacing);

    convolvePanel.setBounds(panelArea.removeFromLeft(panelWidth));

    panelArea.removeFromLeft(panelspacing);

    revebPanel.setBounds(panelArea.removeFromLeft(panelWidth));
    panelArea.removeFromLeft(panelspacing);*/

    //equalizerPanel.setBounds(panelArea);

    //equalizerPanel.setBounds(panelArea.removeFromLeft(panelWidth));




    //delayPanel.setBounds (getLocalBounds());



