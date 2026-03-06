/*
  ==============================================================================
    EQModuleSlotEditor.cpp
  ==============================================================================
*/

#include "EQModuleSlotEditor.h"

EQModuleSlotEditor::EQModuleSlotEditor(
    int cIndex,
    int sIndex,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& state)

    : BaseModuleSlotEditor(
        cIndex,
        sIndex,
        info,
        p,
        state)
{
    buildEditor(info);
    startTimerHz(60);
}

void EQModuleSlotEditor::buildEditor(const SlotInfo& info)
{
    mod = dynamic_cast<EQModule*>(
        processor.slots[chainIndex][slotIndex]->get());

    display = std::make_unique<EQDisplayComponent>(*mod);
    addAndMakeVisible(*display);

    panel = std::make_unique<EQPanel>();
    panel->attachToAPVTS(processor.apvts, slotID);
    addAndMakeVisible(*panel);
}

void EQModuleSlotEditor::layoutEditor(juce::Rectangle<int>& r)
{
    auto displayArea = r.removeFromLeft(200);
    if (display) display->setBounds(displayArea.reduced(5));
    if (panel)   panel->setBounds(r);
}

void EQModuleSlotEditor::timerCallback()
{
    if (mod->fftReady)
    {
        display->pushSamples(mod->fftBuffer,
            mod->fftSize);

        mod->fftReady = false;
    }
}