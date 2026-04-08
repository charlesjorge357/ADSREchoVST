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
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff252323));
}

void CustomLNF::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
    
    auto cornerSize = 6.0f;
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

    /*auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                      .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);*/

   /* if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.contrasting (shouldDrawButtonAsDown ? 0.2f : 0.05f);*/

    juce::Colour baseColour;

    if (!button.isEnabled()) {
        baseColour = juce::Colour(0xff252323).withAlpha(0.5f);
    }
    else if (shouldDrawButtonAsDown) {
        baseColour = juce::Colour(0xff1A1E22);
    }
    else if (shouldDrawButtonAsHighlighted) {
        baseColour = juce::Colour(0xff3A3F47);
    }
    else {
        baseColour = juce::Colour(0xff252323);
    }

    g.setColour (baseColour);



    auto flatOnLeft   = button.isConnectedOnLeft();
    auto flatOnRight  = button.isConnectedOnRight();
    auto flatOnTop    = button.isConnectedOnTop();
    auto flatOnBottom = button.isConnectedOnBottom();

    if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
    {
        juce::Path path;
        path.addRoundedRectangle (bounds.getX(), bounds.getY(),
                                  bounds.getWidth(), bounds.getHeight(),
                                  cornerSize, cornerSize,
                                  ! (flatOnLeft  || flatOnTop),
                                  ! (flatOnRight || flatOnTop),
                                  ! (flatOnLeft  || flatOnBottom),
                                  ! (flatOnRight || flatOnBottom));

        g.fillPath (path);

        g.setColour (button.findColour (juce::ComboBox::outlineColourId));
        g.strokePath (path, juce::PathStrokeType (1.0f));
    }
    else
    {
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }
}

void CustomLNF::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    // Dark background for the popup
    g.fillAll(juce::Colour(0xff252323));
    
    // Border
    g.setColour(juce::Colours::white);
    g.drawRect(0, 0, width, height, 1);
}

void CustomLNF::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
    const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColourToUse) {

        if (isSeparator)
    {
        auto r  = area.reduced (5, 0);
        r.removeFromTop (juce::roundToInt(((float) r.getHeight() * 0.5f) - 0.5f));

        g.setColour (findColour (juce::PopupMenu::textColourId).withAlpha (0.3f));
        g.fillRect (r.removeFromTop (1));
    }
    else
    {
        auto textColour = (textColourToUse == nullptr ? findColour (juce::PopupMenu::textColourId)
                                                      : *textColourToUse);

        auto r  = area.reduced (1);

        if (isHighlighted && isActive)
        {
            g.setColour (juce::Colours::grey);
            g.fillRect (r);

            /*g.setColour (findColour (PopupMenu::highlightedTextColourId));*/
            g.setColour(juce::Colour(0xffF3F2EC)); 
        }
        else
        {
            g.setColour (textColour.withMultipliedAlpha (isActive ? 1.0f : 0.5f));
        }

        r.reduce (juce::jmin (5, area.getWidth() / 20), 0);

        auto font = getPopupMenuFont();

        auto maxFontHeight = (float) r.getHeight() / 1.3f;

        if (font.getHeight() > maxFontHeight)
            font.setHeight (maxFontHeight);

        g.setFont (font);

        auto iconArea = r.removeFromLeft (juce::roundToInt (maxFontHeight)).toFloat();

        if (icon != nullptr)
        {
            icon->drawWithin (g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
            r.removeFromLeft (juce::roundToInt (maxFontHeight * 0.5f));
        }
        else if (isTicked)
        {
            auto tick = getTickShape (1.0f);
            g.fillPath (tick, tick.getTransformToScaleToFit (iconArea.reduced (iconArea.getWidth() / 5, 0).toFloat(), true));
        }

        if (hasSubMenu)
        {
            auto arrowH = 0.6f * getPopupMenuFont().getAscent();

            auto x = static_cast<float> (r.removeFromRight ((int) arrowH).getX());
            auto halfH = static_cast<float> (r.getCentreY());

            juce::Path path;
            path.startNewSubPath (x, halfH - arrowH * 0.5f);
            path.lineTo (x + arrowH * 0.6f, halfH);
            path.lineTo (x, halfH + arrowH * 0.5f);

            g.strokePath (path, juce::PathStrokeType (2.0f));
        }

        r.removeFromRight (3);
        g.drawFittedText (text, r, juce::Justification::centredLeft, 1);

        if (shortcutKeyText.isNotEmpty())
        {
            auto f2 = font;
            f2.setHeight (f2.getHeight() * 0.75f);
            f2.setHorizontalScale (0.95f);
            g.setFont (f2);

            g.drawText (shortcutKeyText, r, juce::Justification::centredRight, true);
        }
    }

}

void CustomLNF::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& edit) {

    auto cornerSize = 5.0f;

    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, cornerSize, 1.0f);
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

static juce::Typeface::Ptr s_designer   = juce::Typeface::createSystemTypefaceFor(BinaryData::Designer_otf,               BinaryData::Designer_otfSize);
static juce::Typeface::Ptr s_animaReg   = juce::Typeface::createSystemTypefaceFor(BinaryData::FtAnimaRegularz453_otf,      BinaryData::FtAnimaRegularz453_otfSize);
static juce::Typeface::Ptr s_animaMed   = juce::Typeface::createSystemTypefaceFor(BinaryData::FtAnimaMediumj8yG_otf,       BinaryData::FtAnimaMediumj8yG_otfSize);
static juce::Typeface::Ptr s_hussarBold = juce::Typeface::createSystemTypefaceFor(BinaryData::HussarBoldObliqueOneoPKq_otf, BinaryData::HussarBoldObliqueOneoPKq_otfSize);
static juce::Typeface::Ptr s_guavine    = juce::Typeface::createSystemTypefaceFor(BinaryData::GuavineDemoRegular1jGgL_otf,  BinaryData::GuavineDemoRegular1jGgL_otfSize);

juce::Typeface::Ptr CustomLNF::getTypefaceForFont(const juce::Font& f)
{
    switch (activeFont)
    {
        case AppFont::AnimaRegular: return s_animaReg;
        case AppFont::AnimaMedium:  return s_animaMed;
        case AppFont::HussarBold:   return s_hussarBold;
        case AppFont::Guavine:      return s_guavine;
        default:                    return s_designer;
    }
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
