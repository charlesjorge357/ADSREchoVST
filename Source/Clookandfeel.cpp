/*
  ==============================================================================

    Clookandfeel.cpp
    Created: 2 Feb 2026 1:35:08am
    Author:  ferna

  ==============================================================================
*/

#include "Clookandfeel.h"


juce::Typeface::Ptr CustomLNF::getTypefaceForFont(const juce::Font& f)
{
    static auto myFont = juce::Typeface::createSystemTypefaceFor(BinaryData::Designer_otf, BinaryData::Designer_otfSize);

    return myFont;
}

void CustomLNF::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour (juce::Slider::rotarySliderFillColourId);

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);

    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin (8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f - 4.0f;

    g.setColour(juce::Colour(0xff111111));
    g.fillEllipse(bounds);

    auto innerBounds = bounds.reduced(15);
    g.setColour(juce::Colour(0xff343434));
    g.fillEllipse(innerBounds);


    juce::Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(),
                                 bounds.getCentreY(),
                                 arcRadius,
                                 arcRadius,
                                 0.0f,
                                 rotaryStartAngle,
                                 rotaryEndAngle,
                                 true);

    g.setColour (outline);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(),
                                bounds.getCentreY(),
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotaryStartAngle,
                                toAngle,
                                true);

        g.setColour (fill);
        g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    auto thumbWidth = lineW * 2.0f;
    juce::Point<float> thumbPoint (bounds.getCentreX() + (arcRadius - 11.0) * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                             bounds.getCentreY() + (arcRadius - 11.0) * std::sin (toAngle - juce::MathConstants<float>::halfPi));

    g.setColour (slider.findColour (juce::Slider::thumbColourId));
   // g.fillEllipse (juce::Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));
    //g.drawLine (backgroundArc.getBounds().getCentreX(),backgroundArc.getBounds().getCentreY(), thumbPoint.getX(), thumbPoint.getY(), lineW);
    // Calculate the angle for rotation
auto angle = std::atan2(thumbPoint.getY() - bounds.getCentreY(), 
                        thumbPoint.getX() - bounds.getCentreX());

// Create oblong shape
auto oblongWidth = 6.0f;
auto oblongLength = 15.0f;

juce::Path oblong;
oblong.addRoundedRectangle(-oblongWidth * 0.5f, -oblongLength * 0.5f, 
                           oblongWidth, oblongLength, 
                           2.0f);

g.setColour(juce::Colour(0xffF3F2EC));
g.fillPath(oblong, juce::AffineTransform::rotation(toAngle)
                   .translated(thumbPoint.getX(), thumbPoint.getY()));

} 
