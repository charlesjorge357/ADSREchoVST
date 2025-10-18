/*
  ==============================================================================

    PresetManager.h
    Preset management system for factory and user presets

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PresetManager
{
public:
    struct Preset
    {
        juce::String name;
        juce::String category;
        juce::String author;
        juce::String description;
        juce::ValueTree state;  // Complete plugin state
        bool isFactory = false;

        // Metadata
        juce::StringArray tags;
        int version = 1;
        juce::Time dateCreated;
        juce::Time dateModified;
    };

    PresetManager();
    ~PresetManager();

    // Factory preset management
    void loadFactoryPresets();
    std::vector<Preset> getFactoryPresets() const { return factoryPresets; }
    int getNumFactoryPresets() const { return (int)factoryPresets.size(); }

    // User preset management
    bool saveUserPreset(const Preset& preset);
    bool deleteUserPreset(const juce::String& presetName);
    bool renameUserPreset(const juce::String& oldName, const juce::String& newName);
    std::vector<Preset> getUserPresets() const { return userPresets; }
    int getNumUserPresets() const { return (int)userPresets.size(); }

    // All presets
    std::vector<Preset> getAllPresets() const;
    int getTotalNumPresets() const { return getNumFactoryPresets() + getNumUserPresets(); }

    // Current preset
    bool loadPreset(const juce::String& presetName);
    bool loadPreset(int index);
    Preset getCurrentPreset() const { return currentPreset; }
    juce::String getCurrentPresetName() const { return currentPreset.name; }
    bool hasCurrentPreset() const { return !currentPreset.name.isEmpty(); }

    // Navigation
    bool loadNextPreset();
    bool loadPreviousPreset();
    int getCurrentPresetIndex() const;

    // Search and filtering
    std::vector<Preset> searchPresets(const juce::String& query) const;
    std::vector<Preset> getPresetsByCategory(const juce::String& category) const;
    std::vector<Preset> getPresetsByTag(const juce::String& tag) const;
    juce::StringArray getAllCategories() const;
    juce::StringArray getAllTags() const;

    // Preset creation
    Preset createPresetFromCurrentState(
        const juce::String& name,
        const juce::String& category,
        const juce::ValueTree& state
    );

    // File management
    juce::File getUserPresetDirectory() const;
    juce::File getFactoryPresetDirectory() const;
    bool exportPreset(const Preset& preset, const juce::File& destination) const;
    bool importPreset(const juce::File& source);

    // Modified state tracking
    void markCurrentPresetModified(bool modified) { currentPresetModified = modified; }
    bool isCurrentPresetModified() const { return currentPresetModified; }

    // Preset file format
    static juce::String getPresetFileExtension() { return ".adsrpreset"; }

private:
    std::vector<Preset> factoryPresets;
    std::vector<Preset> userPresets;
    Preset currentPreset;
    bool currentPresetModified = false;

    // Factory preset definitions (will be populated)
    void createDefaultFactoryPresets();
    void addFactoryPreset(
        const juce::String& name,
        const juce::String& category,
        const juce::String& description,
        const juce::ValueTree& state
    );

    // File I/O
    bool savePresetToFile(const Preset& preset, const juce::File& file) const;
    std::unique_ptr<Preset> loadPresetFromFile(const juce::File& file) const;
    void scanUserPresetsDirectory();

    // Validation
    bool isPresetNameValid(const juce::String& name) const;
    bool isPresetNameUnique(const juce::String& name) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
