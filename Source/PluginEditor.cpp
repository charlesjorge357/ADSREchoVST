/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

//==============================================================================
ADSREchoAudioProcessorEditor::ADSREchoAudioProcessorEditor (ADSREchoAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Helper lambda to set up sliders
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& labelText,
                              std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment,
                              const juce::String& paramID)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(juce::Slider::Rotary);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);

        addAndMakeVisible(label);
        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(11.0f));
        label.attachToComponent(&slider, false);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, paramID, slider);
    };

    // Section labels
    auto setupSectionLabel = [this](juce::Label& label, const juce::String& text)
    {
        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    };

    setupSectionLabel(globalLabel, "GLOBAL");
    setupSectionLabel(delayLabel, "DELAY");
    setupSectionLabel(reverbLabel, "REVERB (Active)");
    setupSectionLabel(compressorLabel, "COMPRESSOR");
    setupSectionLabel(eqLabel, "EQUALIZER");
    setupSectionLabel(convolutionLabel, "CONVOLUTION");

    // Global
    setupSlider(outputGainSlider, outputGainLabel, "Output", outputGainAttachment, ParameterIDs::outputGain);
    setupSlider(dryWetMixSlider, dryWetMixLabel, "Dry/Wet", dryWetMixAttachment, ParameterIDs::dryWetMix);

    // Delay
    setupSlider(delayTimeLeftSlider, delayTimeLeftLabel, "Time L", delayTimeLeftAttachment, ParameterIDs::delayTimeLeft);
    setupSlider(delayTimeRightSlider, delayTimeRightLabel, "Time R", delayTimeRightAttachment, ParameterIDs::delayTimeRight);
    setupSlider(delayFeedbackSlider, delayFeedbackLabel, "Feedback", delayFeedbackAttachment, ParameterIDs::delayFeedback);
    setupSlider(delayMixSlider, delayMixLabel, "Mix", delayMixAttachment, ParameterIDs::delayMix);

    // Reverb
    setupSlider(reverbSizeSlider, reverbSizeLabel, "Size", reverbSizeAttachment, ParameterIDs::reverbSize);
    setupSlider(reverbDampingSlider, reverbDampingLabel, "Damping", reverbDampingAttachment, ParameterIDs::reverbDamping);
    setupSlider(reverbWidthSlider, reverbWidthLabel, "Width", reverbWidthAttachment, ParameterIDs::reverbWidth);
    setupSlider(reverbMixSlider, reverbMixLabel, "Mix", reverbMixAttachment, ParameterIDs::reverbMix);
    setupSlider(reverbPreDelaySlider, reverbPreDelayLabel, "Pre-Delay", reverbPreDelayAttachment, ParameterIDs::reverbPreDelay);

    // Compressor
    setupSlider(compressorThresholdSlider, compressorThresholdLabel, "Threshold", compressorThresholdAttachment, ParameterIDs::compressorThreshold);
    setupSlider(compressorRatioSlider, compressorRatioLabel, "Ratio", compressorRatioAttachment, ParameterIDs::compressorRatio);
    setupSlider(compressorAttackSlider, compressorAttackLabel, "Attack", compressorAttackAttachment, ParameterIDs::compressorAttack);
    setupSlider(compressorReleaseSlider, compressorReleaseLabel, "Release", compressorReleaseAttachment, ParameterIDs::compressorRelease);

    // EQ
    setupSlider(eqLowFreqSlider, eqLowFreqLabel, "Low Freq", eqLowFreqAttachment, ParameterIDs::eqLowFreq);
    setupSlider(eqLowGainSlider, eqLowGainLabel, "Low Gain", eqLowGainAttachment, ParameterIDs::eqLowGain);
    setupSlider(eqMidFreqSlider, eqMidFreqLabel, "Mid Freq", eqMidFreqAttachment, ParameterIDs::eqMidFreq);
    setupSlider(eqMidGainSlider, eqMidGainLabel, "Mid Gain", eqMidGainAttachment, ParameterIDs::eqMidGain);
    setupSlider(eqHighFreqSlider, eqHighFreqLabel, "High Freq", eqHighFreqAttachment, ParameterIDs::eqHighFreq);
    setupSlider(eqHighGainSlider, eqHighGainLabel, "High Gain", eqHighGainAttachment, ParameterIDs::eqHighGain);

    // Convolution
    setupSlider(convolutionMixSlider, convolutionMixLabel, "Mix", convolutionMixAttachment, ParameterIDs::convolutionMix);

    setSize(850, 520);
    setResizable(true, true);
    setResizeLimits(700, 400, 1400, 800);
}

ADSREchoAudioProcessorEditor::~ADSREchoAudioProcessorEditor()
{
}

//==============================================================================
void ADSREchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill background with gradient
    g.fillAll(juce::Colour(0xff1a1a1a));

    auto bounds = getLocalBounds();

    // Draw title bar
    auto titleArea = bounds.removeFromTop(40);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2d2d2d), 0, 0,
                                            juce::Colour(0xff1a1a1a), 0, 40, false));
    g.fillRect(titleArea);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("ADSREcho", titleArea, juce::Justification::centred);

    // Draw separator lines between sections
    g.setColour(juce::Colour(0xff404040));
    g.drawLine(0, 40, getWidth(), 40, 2.0f);
}

void ADSREchoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(40);  // Skip title
    area.reduce(10, 10);

    const int knobSize = 85;
    const int sectionHeaderHeight = 20;
    const int labelHeight = 15;
    const int rowHeight = knobSize + labelHeight + 5;

    // Helper lambda to layout a row of knobs
    auto layoutKnobs = [&](juce::Label& sectionLabel, std::vector<juce::Slider*> sliders)
    {
        auto sectionArea = area.removeFromTop(sectionHeaderHeight);
        sectionLabel.setBounds(sectionArea);

        area.removeFromTop(labelHeight);  // Space for knob labels

        auto knobRow = area.removeFromTop(knobSize + 20);  // knob + text box
        for (auto* slider : sliders)
        {
            auto knobArea = knobRow.removeFromLeft(knobSize);
            slider->setBounds(knobArea);
            knobRow.removeFromLeft(10);  // spacing
        }

        area.removeFromTop(8);  // section spacing
    };

    // Layout sections
    layoutKnobs(globalLabel, {&outputGainSlider, &dryWetMixSlider});
    layoutKnobs(delayLabel, {&delayTimeLeftSlider, &delayTimeRightSlider, &delayFeedbackSlider, &delayMixSlider});
    layoutKnobs(reverbLabel, {&reverbSizeSlider, &reverbDampingSlider, &reverbWidthSlider, &reverbMixSlider, &reverbPreDelaySlider});
    layoutKnobs(compressorLabel, {&compressorThresholdSlider, &compressorRatioSlider, &compressorAttackSlider, &compressorReleaseSlider});
    layoutKnobs(eqLabel, {&eqLowFreqSlider, &eqLowGainSlider, &eqMidFreqSlider, &eqMidGainSlider, &eqHighFreqSlider, &eqHighGainSlider});
    layoutKnobs(convolutionLabel, {&convolutionMixSlider});
}
