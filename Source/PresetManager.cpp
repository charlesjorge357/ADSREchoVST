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
    defaultPreset.dateCreated = juce::Time::getCurrentTime();
    defaultPreset.dateModified = juce::Time::getCurrentTime();

    factoryPresets.push_back(defaultPreset);
}

bool PresetManager::saveUserPreset(const Preset& preset)
{
    // TODO: Implement user preset saving to disk
    userPresets.push_back(preset);
    return true;
}

bool PresetManager::deleteUserPreset(const juce::String& presetName)
{
    auto it = std::find_if(userPresets.begin(), userPresets.end(),
        [&presetName](const Preset& p) { return p.name == presetName; });

    if (it != userPresets.end())
    {
        userPresets.erase(it);
        return true;
    }
    return false;
}

bool PresetManager::renameUserPreset(const juce::String& oldName, const juce::String& newName)
{
    auto it = std::find_if(userPresets.begin(), userPresets.end(),
        [&oldName](const Preset& p) { return p.name == oldName; });

    if (it != userPresets.end())
    {
        it->name = newName;
        it->dateModified = juce::Time::getCurrentTime();
        return true;
    }
    return false;
}

std::vector<PresetManager::Preset> PresetManager::getAllPresets() const
{
    std::vector<Preset> allPresets = factoryPresets;
    allPresets.insert(allPresets.end(), userPresets.begin(), userPresets.end());
    return allPresets;
}

bool PresetManager::loadPreset(const juce::String& presetName)
{
    // Search in factory presets
    for (const auto& preset : factoryPresets)
    {
        if (preset.name == presetName)
        {
            currentPreset = preset;
            return true;
        }
    }

    // Search in user presets
    for (const auto& preset : userPresets)
    {
        if (preset.name == presetName)
        {
            currentPreset = preset;
            return true;
        }
    }

    return false;
}

bool PresetManager::loadPreset(int index)
{
    auto allPresets = getAllPresets();
    if (index >= 0 && index < static_cast<int>(allPresets.size()))
    {
        currentPreset = allPresets[index];
        return true;
    }
    return false;
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

int PresetManager::getNextPresetIndex() const
{
    auto allPresets = getAllPresets();
    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        if (allPresets[i].name == currentPreset.name)
        {
            return (static_cast<int>(i) + 1) % static_cast<int>(allPresets.size());
        }
    }
    return 0;
}

int PresetManager::getPreviousPresetIndex() const
{
    auto allPresets = getAllPresets();
    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        if (allPresets[i].name == currentPreset.name)
        {
            return (static_cast<int>(i) - 1 + static_cast<int>(allPresets.size())) % static_cast<int>(allPresets.size());
        }
    }
    return 0;
}
