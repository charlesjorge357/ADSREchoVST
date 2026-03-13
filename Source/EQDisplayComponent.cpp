/*
  ==============================================================================

    EQDisplayComponent.cpp
    Created: 2 Mar 2026 3:20:00pm
    Author:  Crabnebuelar

  ==============================================================================
*/

#include "EQDisplayComponent.h"

EQDisplayComponent::EQDisplayComponent()
    : fft(fftOrder),
    window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    startTimerHz(60);
    smoothedFFT.resize(fftSize / 2, -100.0f);
}

void EQDisplayComponent::pushSamples(const float* samples, int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        fifo[fifoIndex++] = samples[i];

        if (fifoIndex == fftSize)
        {
            memcpy(fftData, fifo, sizeof(float) * fftSize);
            ready = true;
            fifoIndex = 0;
        }
    }
}

void EQDisplayComponent::pushEQData(float sampleRate, std::vector<float> curveDb)
{
    cachedSampleRate  = sampleRate;
    cachedEQCurveDb   = std::move(curveDb);
}

void EQDisplayComponent::timerCallback()
{
    if (ready)
    {
        drawNextFrame();
        ready = false;
        repaint();
    }
}

void EQDisplayComponent::drawNextFrame()
{
    window.multiplyWithWindowingTable(fftData, fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData);

    for (int i = 0; i < fftSize / 2; i++) {
        fftData[i] /= (float)fftSize;

        float newValue = fftData[i];

        float decay = 0.78f;

        smoothedFFT[i] = std::max(newValue, smoothedFFT[i] * decay);
    }
}

void EQDisplayComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    int freqLabelCutoff = 15;
    int dbLabelCutoff = 20;
    auto freqLabelBounds = bounds.removeFromBottom(freqLabelCutoff);
    auto dbLabelBounds = bounds.removeFromLeft(dbLabelCutoff);

    g.fillAll(juce::Colours::black);

    // ==============================
    // 1) DRAW GRID
    // ==============================
    g.setColour(juce::Colours::dimgrey);
    g.setFont(8.0f);

    //  Frequency Grid
    for (float freq : { 20.f, 100.f, 1000.f, 10000.f})
    {
        float normX = juce::mapFromLog10(freq, 15.f, 25000.f);
        int x = normX * bounds.getWidth() + dbLabelCutoff;
        juce::String freqString = juce::String((int)freq / 1000 + (int)freq % 1000);
        freqString += ((int)freq >= 1000) ? "k" : "";
        g.setColour(juce::Colours::dimgrey);
        g.drawVerticalLine(x, 0, bounds.getHeight());
        g.setColour(juce::Colours::cyan);
        g.drawText(freqString, x-10, freqLabelBounds.getY(), 20, 15, juce::Justification::centredBottom);
    }

    // Decibel Grid
    for (float gain : { -20.f, -10.f, 0.f, 10.f, 20.f})
    {
        float y = juce::jmap(gain, -24.0f, 24.0f, (float)bounds.getBottom(), (float)bounds.getY());
        juce::String dbString = juce::String((int)gain);
        
        
        g.setColour(juce::Colours::dimgrey);
        g.drawHorizontalLine(y, 15, bounds.getWidth() + dbLabelCutoff);
        g.setColour(juce::Colours::cyan);
        g.drawText(dbString, 0, y-10, dbLabelCutoff-2, 20, juce::Justification::centredRight);
    }

    // ==============================
    // 2) DRAW SPECTRUM
    // ==============================

    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(dbLabelCutoff, bounds.getBottom());

    int width = bounds.getWidth();

    float minFreq = 15.0f;
    float maxFreq = 25000.0f;
    float sampleRate = cachedSampleRate;

    for (int x = 0; x < width; x++)
    {
        float normX = x / (float)width;

        float freq = minFreq * std::pow(maxFreq / minFreq, normX);

        float bin = freq * fftSize / sampleRate;

        int center = (int)bin;

        float mag = 0.0f;
        int radius = 1;

        for (int k = -radius; k <= radius; k++)
        {
            int idx = juce::jlimit(
                0,
                fftSize / 2,
                center + k);

            mag += smoothedFFT[idx];
        }

        mag /= (2 * radius + 1);

        float magNext = 0.0f;

        for (int k = -radius; k <= radius; k++)
        {
            int idx = juce::jlimit(
                0,
                fftSize / 2,
                center + 1 + k);

            magNext += smoothedFFT[idx];
        }

        magNext /= (2 * radius + 1);

        float frac = bin - center;

        float finalMag =
            mag * (1 - frac) + magNext * frac;

        float level = juce::Decibels::gainToDecibels(
            finalMag, -80.0f);

        float y = juce::jmap(level,
            -80.0f, 10.0f,
            (float)bounds.getBottom(),
            (float)bounds.getY());

        spectrumPath.lineTo(x + dbLabelCutoff, y);
    }

    spectrumPath.lineTo(bounds.getWidth() + dbLabelCutoff, bounds.getBottom());
    spectrumPath.closeSubPath();

    g.setColour(juce::Colours::yellow.withAlpha(0.4f));
    g.fillPath(spectrumPath);

    // ==============================
    // 3) DRAW EQ CURVE
    // ==============================

    juce::Path eqPath;

    const int N = (int)cachedEQCurveDb.size();

    for (int x = 0; x < width; x++)
    {
        float db;
        if (N > 0)
        {
            int idx = juce::jlimit(0, N - 1, x * N / juce::jmax(1, width));
            db = cachedEQCurveDb[idx];
        }
        else
        {
            db = 0.0f; // flat line until first push
        }

        float y = juce::jmap(db,
            -24.0f, 24.0f,
            (float)bounds.getBottom(),
            (float)bounds.getY());

        if (x == 0)
            eqPath.startNewSubPath(x + dbLabelCutoff, y);
        else
            eqPath.lineTo(x + dbLabelCutoff, y);
    }

    g.setColour(juce::Colours::cyan);
    g.strokePath(eqPath, juce::PathStrokeType(2.f));


    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(freqLabelBounds);
    g.fillRect(dbLabelBounds);

    g.setColour(juce::Colours::cyan);
    for (float freq : { 20.f, 100.f, 1000.f, 10000.f})
    {
        float normX = juce::mapFromLog10(freq, 15.f, 25000.f);
        int x = normX * bounds.getWidth() + dbLabelCutoff;
        juce::String freqString = juce::String((int)freq / 1000 + (int)freq % 1000);
        freqString += ((int)freq >= 1000) ? "k" : "";

        g.drawText(freqString, x - 10, freqLabelBounds.getY(), 20, 15, juce::Justification::centredBottom);
    }

    for (float gain : { -20.f, -10.f, 0.f, 10.f, 20.f})
    {
        float y = juce::jmap(gain, -24.0f, 24.0f, (float)bounds.getBottom(), (float)bounds.getY());
        juce::String dbString = juce::String((int)gain);

        g.drawText(dbString, 0, y - 10, dbLabelCutoff - 2, 20, juce::Justification::centredRight);
    }
}