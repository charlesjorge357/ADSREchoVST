/*
  ==============================================================================

    CompressorModuleSlotEditor.cpp

  ==============================================================================
*/

#include "CompressorModuleSlotEditor.h"

CompressorModuleSlotEditor::CompressorModuleSlotEditor(
    int cIndex,
    int sIndex,
    const SlotInfo& info,
    ADSREchoAudioProcessor& p,
    juce::AudioProcessorValueTreeState& apvtsRef)
    : BaseModuleSlotEditor(cIndex, sIndex, info, p, apvtsRef)
{
    buildEditor(info);
    startTimerHz(60);
}

// ---------------------------------------------------------------------------
// Build controls from parameter list (identical pattern to EQModuleSlotEditor)
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::buildEditor(const SlotInfo& info)
{
    mod = dynamic_cast<CompressorModule*>(
        processor.slots[chainIndex][slotIndex]->get());

    display = std::make_unique<CompressorDisplayComponent>(*mod);
    addAndMakeVisible(*display);

    panel = std::make_unique<CompressorPanel>();
    panel->attachToAPVTS(processor.apvts, slotID);
    addAndMakeVisible(*panel);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::layoutEditor(juce::Rectangle<int>& r)
{
    auto displayArea = r.removeFromLeft(120);
    if (display) display->setBounds(displayArea.reduced(5));
    if (panel)   panel->setBounds(r);
}

// ---------------------------------------------------------------------------
// Timer - poll meterReady, push values to display (same as EQModuleSlotEditor)
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::timerCallback()
{
    if (mod && mod->meterReady.exchange(false, std::memory_order_acquire))
    {
        display->pushMeterValues(
            mod->inputLevelDb.load(std::memory_order_relaxed),
            mod->gainReductionDb.load(std::memory_order_relaxed));
    }
}




