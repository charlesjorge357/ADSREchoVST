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

    display = std::make_unique<CompressorDisplayComponent>();
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
    int displayHeight = juce::jmax(100, static_cast<int>(r.getHeight() * 0.5f));
    if (display) display->setBounds(r.removeFromTop(displayHeight).reduced(4));
    r.removeFromTop(4);
    if (panel)   panel->setBounds(r);
}

// ---------------------------------------------------------------------------
// Timer - poll meterReady, push values to display (same as EQModuleSlotEditor)
// ---------------------------------------------------------------------------

void CompressorModuleSlotEditor::timerCallback()
{
    // Guard against dangling pointer: if the module in this slot was replaced
    // (e.g. by a preset switch), null out mod and wait for rebuildModuleEditors().
    if (mod == nullptr || processor.slots[chainIndex][slotIndex]->get() != mod)
    {
        mod = nullptr;
        return;
    }

    if (mod->meterReady.exchange(false, std::memory_order_acquire))
    {
        display->pushMeterValues(
            mod->inputLevelDb.load(std::memory_order_relaxed),
            mod->gainReductionDb.load(std::memory_order_relaxed),
            mod->getThresholdDb());
    }
}




