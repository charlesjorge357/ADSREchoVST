// ==============================================================================
// PresetManager.h - Save and load plugin presets as .adecho files
// Presets are stored alongside the plugin binary in a "Presets" folder,
// mirroring the same path strategy used by IRBank for the "IRs" folder.
// ==============================================================================
#pragma once

#if __has_include("JuceHeader.h")
  #include "JuceHeader.h"
#else
  #include <juce_core/juce_core.h>
#endif

// Forward declaration to avoid circular include
class ADSREchoAudioProcessor;

class PresetManager
{
public:
    static constexpr const char* presetExtension = ".adecho";

    explicit PresetManager (ADSREchoAudioProcessor& processor);

    // Returns the platform-aware Presets directory (sibling of IRs folder).
    // Creates it if it doesn't exist.
    static juce::File getPresetsDirectory();

    // Saves the current processor state as <name>.adecho in the Presets folder.
    // Returns true on success.
    bool savePreset (const juce::String& name);

    // Loads a preset from a .adecho file and applies it to the processor.
    // Returns true on success.
    bool loadPreset (const juce::File& file);

    // Returns all .adecho files found in the Presets directory, sorted by name.
    juce::Array<juce::File> getPresetFiles() const;

    // Convenience: returns just the preset names (filename without extension).
    juce::StringArray getPresetNames() const;

private:
    ADSREchoAudioProcessor& processor;
};
