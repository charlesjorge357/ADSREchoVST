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

    display = std::make_unique<EQDisplayComponent>();
    addAndMakeVisible(*display);

    panel = std::make_unique<EQPanel>();
    panel->attachToAPVTS(processor.apvts, slotID);
    addAndMakeVisible(*panel);
}

void EQModuleSlotEditor::layoutEditor(juce::Rectangle<int>& r)
{
    if (display) display->setBounds(r.removeFromTop(150).reduced(4));
    r.removeFromTop(4);
    if (panel)   panel->setBounds(r);
}

void EQModuleSlotEditor::timerCallback()
{
    // Guard against dangling pointer: if the module in this slot was replaced
    // (e.g. by a preset switch), null out mod and wait for rebuildModuleEditors().
    if (mod == nullptr || processor.slots[chainIndex][slotIndex]->get() != mod)
    {
        mod = nullptr;
        return;
    }

    if (mod->fftReady)
    {
        display->pushSamples(mod->fftBuffer, mod->fftSize);
        mod->fftReady = false;
    }

    // Push pre-computed EQ curve so paint() never needs a module pointer.
    {
        constexpr int N = 512;
        std::vector<float> curve(N);
        for (int i = 0; i < N; i++)
        {
            float normX = (float)i / (float)(N - 1);
            float freq  = juce::mapToLog10(normX, 20.0f, 20000.0f);
            curve[i]    = juce::Decibels::gainToDecibels(
                              mod->getMagnitudeForFrequency(freq));
        }
        display->pushEQData(mod->getSampleRate(), std::move(curve));
    }
}