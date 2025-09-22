#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class ADSREchoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ADSREchoAudioProcessorEditor(ADSREchoAudioProcessor&);
    ~ADSREchoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ADSREchoAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSREchoAudioProcessorEditor)
};
