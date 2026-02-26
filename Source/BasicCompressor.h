// BasicCompressor.h
// Optical-style compressor inspired by LA-2A program-dependent behaviour,
// SSL G-Bus glue, and Neve 33609 soft-knee musicality.
// Linked stereo detector, shared gain reduction applied equally to both channels.

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>
#endif

class BasicCompressor
{
public:
    BasicCompressor();
    ~BasicCompressor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();

    void setThreshold(float dBFS);     // -60 to 0 dBFS
    void setRatio(float ratio);        // 1.0 to 20.0
    void setAttack(float ms);          // 1 to 200 ms
    void setRelease(float ms);         // 10 to 2000 ms
    void setInputGain(float dB);       // -18 to +18 dB
    void setOutputGain(float dB);      // -18 to +18 dB

private:
    // -----------------------------------------------------------------------
    // Optical cell simulation
    // The LA-2A's T4 optical attenuator responds faster to high-level signals
    // and has a frequency-weighted detector (more sensitive to low-mids).
    // We model this with program-dependent time constants and a pre-detector
    // low-mid emphasis filter.
    // -----------------------------------------------------------------------

    // Linked RMS envelope follower state (single detector for both channels)
    double envelopeState = 0.0;

    // Smooth gain reduction state (separate smoother to avoid zipper noise
    // when the envelope changes quickly — models the optical cell's lag)
    double gainReductionState = 0.0;

    // Pre-detector low-mid emphasis filter (models optical cell sensitivity)
    // Simple single-pole shelving boost around 300-800 Hz
    juce::dsp::FirstOrderTPTFilter<float> detectorFilterL;
    juce::dsp::FirstOrderTPTFilter<float> detectorFilterR;

    // SSL-style soft-knee gain computer
    // Returns gain reduction in dB for a given input level in dB
    double computeGainReductionDb(double inputDb) const;

    // Program-dependent time constants (LA-2A inspired)
    // Attack gets faster for louder signals, release gets longer for sustained
    double computeAttackCoeff(double inputDb) const;
    double computeReleaseCoeff(double inputDb) const;

    // Parameters
    float thresholdDb  = -18.0f;
    float ratio        = 4.0f;
    float attackMs     = 10.0f;
    float releaseMs    = 100.0f;
    float inputGainDb  = 0.0f;
    float outputGainDb = 0.0f;

    // Soft knee width (fixed, Neve-style gentle knee)
    static constexpr float kneeWidthDb = 6.0f;

    double sampleRate = 44100.0;

    // Pre-computed coefficients (updated on param change)
    double attackCoeff  = 0.0;
    double releaseCoeff = 0.0;

    void updateTimeCoefficients();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicCompressor)
};