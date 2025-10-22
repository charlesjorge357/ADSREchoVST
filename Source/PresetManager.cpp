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

bool PresetManager::loadNextPreset()
{
    auto allPresets = getAllPresets();
    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        if (allPresets[i].name == currentPreset.name)
        {
            int nextIndex = (static_cast<int>(i) + 1) % static_cast<int>(allPresets.size());
            return loadPreset(nextIndex);
        }
    }
    return false;
}

bool PresetManager::loadPreviousPreset()
{
    auto allPresets = getAllPresets();
    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        if (allPresets[i].name == currentPreset.name)
        {
            int prevIndex = (static_cast<int>(i) - 1 + static_cast<int>(allPresets.size())) % static_cast<int>(allPresets.size());
            return loadPreset(prevIndex);
        }
    }
    return false;
}

int PresetManager::getCurrentPresetIndex() const
{
    auto allPresets = getAllPresets();
    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        if (allPresets[i].name == currentPreset.name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<PresetManager::Preset> PresetManager::getPresetsByCategory(const juce::String& category) const
{
    std::vector<Preset> results;
    for (const auto& preset : getAllPresets())
    {
        if (preset.category == category)
            results.push_back(preset);
    }
    return results;
}

std::vector<PresetManager::Preset> PresetManager::getPresetsByTag(const juce::String& tag) const
{
    std::vector<Preset> results;
    for (const auto& preset : getAllPresets())
    {
        for (const auto& presetTag : preset.tags)
        {
            if (presetTag == tag)
            {
                results.push_back(preset);
                break;
            }
        }
    }
    return results;
}

juce::StringArray PresetManager::getAllCategories() const
{
    juce::StringArray categories;
    for (const auto& preset : getAllPresets())
    {
        if (!categories.contains(preset.category))
            categories.add(preset.category);
    }
    return categories;
}

juce::StringArray PresetManager::getAllTags() const
{
    juce::StringArray tags;
    for (const auto& preset : getAllPresets())
    {
        for (const auto& tag : preset.tags)
        {
            if (!tags.contains(tag))
                tags.add(tag);
        }
    }
    return tags;
}

PresetManager::Preset PresetManager::createPresetFromCurrentState(
    const juce::String& name,
    const juce::String& category,
    const juce::ValueTree& state)
{
    Preset preset;
    preset.name = name;
    preset.category = category;
    preset.author = "User";
    preset.state = state;
    preset.isFactory = false;
    preset.dateCreated = juce::Time::getCurrentTime();
    preset.dateModified = juce::Time::getCurrentTime();
    return preset;
}

juce::File PresetManager::getUserPresetDirectory() const
{
    auto userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return userDir.getChildFile("ADSREcho").getChildFile("Presets");
}

juce::File PresetManager::getFactoryPresetDirectory() const
{
    auto appDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    return appDir.getParentDirectory().getChildFile("Presets");
}

bool PresetManager::exportPreset(const Preset& preset, const juce::File& destination) const
{
    // TODO: Implement preset export
    juce::ignoreUnused(preset, destination);
    return false;
}

bool PresetManager::importPreset(const juce::File& source)
{
    // TODO: Implement preset import
    juce::ignoreUnused(source);
    return false;
}
