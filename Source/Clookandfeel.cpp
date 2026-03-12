/*
  ==============================================================================

    Clookandfeel.cpp
    Created: 2 Feb 2026 1:35:08am
    Author:  ferna

  ==============================================================================
*/

#include "Clookandfeel.h"
#include "BinaryData.h"

CustomLNF::CustomLNF()
{
    // ComboBox colours
   /* setColour(juce::ComboBox::backgroundColourId,  juce::Colour(0xff2A2E35));
    setColour(juce::ComboBox::outlineColourId,     juce::Colour(0xff555555));
    setColour(juce::ComboBox::textColourId,        juce::Colours::white);
    setColour(juce::ComboBox::arrowColourId,       juce::Colours::lightgrey);*/

    // PopupMenu colours (dropdown list)
 /*   setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(0xff2A2E35));
    setColour(juce::PopupMenu::textColourId,                  juce::Colours::white);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff4A90D9));
    setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);*/
}


void CustomLNF::drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) {

    auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 5.0f;
    juce::Rectangle<int> boxBounds (0, 0, width, height);

    g.setColour(juce::Colour(0xff252323));
    //g.setColour (box.findColour (juce::ComboBox::ColourIds(0xff3a86ff)));
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

    g.setColour(juce::Colour(0xfff5f1ed));
    //g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

    juce::Rectangle<int> arrowZone (width - 30, 0, 20, height);
    juce::Path path;
    path.startNewSubPath ((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 2.0f);
    path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
    path.lineTo ((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 2.0f);

    g.setColour(juce::Colour(0xfff2f4f3).withAlpha(box.isEnabled() ? 0.9f : 0.2f));
    g.strokePath (path, juce::PathStrokeType (2.0f));

}

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
