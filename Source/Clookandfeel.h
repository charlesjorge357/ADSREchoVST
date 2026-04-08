/*
  ==============================================================================

    Clookandfeel.h
    Created: 2 Feb 2026 1:35:08am
    Author:  ferna

  ==============================================================================
*/

#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"  // for Projucer
#else // for Cmake
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_audio_formats/juce_audio_formats.h>
  #include <juce_audio_plugin_client/juce_audio_plugin_client.h>
  #include <juce_audio_processors/juce_audio_processors.h>
  #include <juce_audio_utils/juce_audio_utils.h>
  #include <juce_core/juce_core.h>
  #include <juce_data_structures/juce_data_structures.h>
  #include <juce_dsp/juce_dsp.h>
  #include <juce_events/juce_events.h>
  #include <juce_graphics/juce_graphics.h>
  #include <juce_gui_basics/juce_gui_basics.h>
  #include <juce_gui_extra/juce_gui_extra.h>
#endif

enum class AppFont { Designer, AnimaRegular, AnimaMedium, HussarBold, Guavine };

class CustomLNF : public juce::LookAndFeel_V4
{

    public:
        CustomLNF();

        void setFont(AppFont f) { activeFont = f; }

        //custom slider function
        void drawRotarySlider(juce::Graphics &, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&);

        void drawComboBox(juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox&);
        void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor &);

        void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool, bool, bool, bool, bool, const juce::String& text, 
            const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour) override;

        void drawPopupMenuBackground(juce::Graphics& g, int width, int height);

        void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;



        juce::Typeface::Ptr getTypefaceForFont(const juce::Font& f) override;

    private:
        AppFont activeFont = AppFont::Designer;
};
