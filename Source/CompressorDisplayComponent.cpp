/*
  ==============================================================================

    CompressorDisplayComponent.cpp

  ==============================================================================
*/

#include "CompressorDisplayComponent.h"

CompressorDisplayComponent::CompressorDisplayComponent(CompressorModule& modRef)
    : mod(modRef)
{
    // Repaint at 60 Hz (same rate as EQDisplayComponent)
    startTimerHz(60);
}

void CompressorDisplayComponent::pushMeterValues(float inputDb, float grDb)
{
    // Fast-attack / slow-decay smoothing on the UI thread
    // Attack: instant rise; Decay: ~20 dB/s at 60 Hz
    const float decayPerFrame = 20.0f / 60.0f;

    if (inputDb > displayInputDb)
        displayInputDb = inputDb;
    else
        displayInputDb = juce::jmax(displayInputDb - decayPerFrame, inputDb);

    // GR is negative (grDb <= 0).  "More GR" means a lower (more negative) value.
    if (grDb < displayGrDb)
        displayGrDb = grDb;
    else
        displayGrDb = juce::jmin(displayGrDb + decayPerFrame, grDb);

    // Peak hold - input
    if (inputDb >= peakInputDb)
    {
        peakInputDb   = inputDb;
        peakInputHold = peakHoldFrames;
    }
    else if (peakInputHold > 0)
    {
        --peakInputHold;
    }
    else
    {
        peakInputDb = juce::jmax(peakInputDb - decayPerFrame * 0.5f, inputDb);
    }

    // Peak hold - GR (most-negative peak)
    if (grDb <= peakGrDb)
    {
        peakGrDb   = grDb;
        peakGrHold = peakHoldFrames;
    }
    else if (peakGrHold > 0)
    {
        --peakGrHold;
    }
    else
    {
        peakGrDb = juce::jmin(peakGrDb + decayPerFrame * 0.5f, grDb);
    }
}

void CompressorDisplayComponent::timerCallback()
{
    repaint();
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------

void CompressorDisplayComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(4);
    g.fillAll(juce::Colour(0xff1a1a1a));

    const int meterW  = (bounds.getWidth() - 8) / 2;
    auto inputArea = bounds.removeFromLeft(meterW);
    bounds.removeFromLeft(8);
    auto grArea = bounds;

    drawMeter(g, inputArea,
              displayInputDb, peakInputDb,
              juce::Colour(0xff44dd88),
              "IN", false);

    drawMeter(g, grArea,
              displayGrDb, peakGrDb,
              juce::Colour(0xffdd4444),
              "GR", true);

    // ---- Threshold line on the input meter ----
    float thresh = mod.getThresholdDb();
    float threshNorm = dbToNorm(thresh, meterFloor, meterCeil);

    // inputArea has been consumed - recalculate from local bounds
    auto inputRect = getLocalBounds().reduced(4);
    inputRect = inputRect.removeFromLeft(meterW);

    // Reserve space for the label at the bottom (same as drawMeter)
    auto barRect = inputRect.reduced(2);
    barRect.removeFromBottom(16);

    int threshY = barRect.getBottom()
                  - (int)(threshNorm * barRect.getHeight());

    g.setColour(juce::Colours::yellow.withAlpha(0.8f));
    g.drawHorizontalLine(threshY,
                         (float)barRect.getX(),
                         (float)barRect.getRight());
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float CompressorDisplayComponent::dbToNorm(float db, float floor, float ceil) const
{
    return juce::jlimit(0.0f, 1.0f,
        (db - floor) / (ceil - floor));
}

void CompressorDisplayComponent::drawMeter(juce::Graphics& g,
                                           juce::Rectangle<int> area,
                                           float value,
                                           float peak,
                                           juce::Colour colour,
                                           const juce::String& label,
                                           bool isGR) const
{
    auto barRect = area.reduced(2);

    // Label at the bottom
    auto labelRect = barRect.removeFromBottom(16);
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    g.drawText(label, labelRect, juce::Justification::centred);

    // Background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(barRect);

    if (!isGR)
    {
        // Input level: bar grows upward from the bottom
        float norm  = dbToNorm(value, meterFloor, meterCeil);
        int   barH  = (int)(norm * barRect.getHeight());

        // Colour gradient: green -> yellow -> red
        juce::ColourGradient grad(
            juce::Colours::green,   (float)barRect.getX(), (float)barRect.getBottom(),
            juce::Colours::red,     (float)barRect.getX(), (float)barRect.getY(),
            false);
        grad.addColour(0.75, juce::Colours::yellow);

        g.setGradientFill(grad);
        g.fillRect(barRect.withTop(barRect.getBottom() - barH));

        // Peak tick
        float peakNorm = dbToNorm(peak, meterFloor, meterCeil);
        int   peakY    = barRect.getBottom() - (int)(peakNorm * barRect.getHeight());
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillRect(barRect.getX(), peakY - 1, barRect.getWidth(), 2);
    }
    else
    {
        // GR meter: bar grows downward from the top (value is <= 0)
        // grFloor is the most reduction we display (e.g. -30 dB)
        float norm  = dbToNorm(-value, 0.0f, -grFloor); // flip sign so 0 GR = empty
        int   barH  = (int)(norm * barRect.getHeight());

        g.setColour(colour);
        g.fillRect(barRect.withHeight(barH));

        // Peak tick (most-negative GR = most reduction = furthest down)
        float peakNorm = dbToNorm(-peak, 0.0f, -grFloor);
        int   peakY    = barRect.getY() + (int)(peakNorm * barRect.getHeight());
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillRect(barRect.getX(), peakY - 1, barRect.getWidth(), 2);

        // dB scale ticks on the GR meter: 0, -3, -6, -12, -20 dB
        g.setColour(juce::Colours::grey.withAlpha(0.6f));
        g.setFont(9.0f);
        for (float tick : { 0.0f, -3.0f, -6.0f, -12.0f, -20.0f })
        {
            float tickNorm = dbToNorm(-tick, 0.0f, -grFloor);
            int   tickY    = barRect.getY() + (int)(tickNorm * barRect.getHeight());
            g.drawHorizontalLine(tickY,
                                 (float)barRect.getX(),
                                 (float)barRect.getRight());
            g.drawText(juce::String((int)tick) + "dB",
                       barRect.getX(), tickY - 9, barRect.getWidth(), 9,
                       juce::Justification::centredRight);
        }
    }

    // Border
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(barRect);
}