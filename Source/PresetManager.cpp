/*
  ==============================================================================

    PresetManager.cpp
    Preset management system for factory and user presets

  ==============================================================================
*/

#include "PresetManager.h"

PresetManager::PresetManager()
{
}

PresetManager::~PresetManager()
{
}

void PresetManager::loadFactoryPresets()
{
    // TODO: Implement factory preset loading
    // For now, create a simple default preset
    Preset defaultPreset;
    defaultPreset.name = "Default";
    defaultPreset.category = "Factory";
    defaultPreset.author = "ADSR Echo";
    defaultPreset.description = "Clean starting point";
    defaultPreset.isFactory = true;

    factoryPresets.push_back(defaultPreset);
}

bool PresetManager::saveUserPreset(const Preset& preset)
{
    // TODO: Implement user preset saving to disk
    userPresets.push_back(preset);
    return true;
}

bool PresetManager::loadUserPreset(const juce::String& presetName)
{
    // TODO: Implement user preset loading
    juce::ignoreUnused(presetName);
    return false;
}

std::vector<PresetManager::Preset> PresetManager::getFactoryPresets() const
{
    return factoryPresets;
}

std::vector<PresetManager::Preset> PresetManager::getUserPresets() const
{
    return userPresets;
}

std::vector<PresetManager::Preset> PresetManager::searchPresets(const juce::String& query) const
{
    std::vector<Preset> results;

    // Search through all presets
    for (const auto& preset : factoryPresets)
    {
        if (preset.name.containsIgnoreCase(query) ||
            preset.category.containsIgnoreCase(query) ||
            preset.description.containsIgnoreCase(query))
        {
            results.push_back(preset);
        }
    }

    for (const auto& preset : userPresets)
    {
        if (preset.name.containsIgnoreCase(query) ||
            preset.category.containsIgnoreCase(query) ||
            preset.description.containsIgnoreCase(query))
        {
            results.push_back(preset);
        }
    }

    return results;
}
