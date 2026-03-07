// ==============================================================================
// PresetManager.cpp
// ==============================================================================
#include "PresetManager.h"
#include "PluginProcessor.h"

PresetManager::PresetManager (ADSREchoAudioProcessor& p)
    : processor (p)
{
}

//==============================================================================
juce::File PresetManager::getPresetsDirectory()
{
    auto pluginPath = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

    juce::File presetsFolder;

#if JUCE_WINDOWS
    presetsFolder = pluginPath.getParentDirectory().getChildFile ("Presets");
#elif JUCE_MAC
    presetsFolder = pluginPath.getParentDirectory()
                              .getParentDirectory()
                              .getChildFile ("Resources")
                              .getChildFile ("Presets");
#else
    presetsFolder = pluginPath.getParentDirectory().getChildFile ("Presets");
#endif

    // Development fallback: walk up looking for Source/Presets
    if (!presetsFolder.exists())
    {
        auto searchDir = pluginPath.getParentDirectory();

        for (int i = 0; i < 10; ++i)
        {
            auto candidate = searchDir.getChildFile ("Source").getChildFile ("Presets");

            if (candidate.isDirectory())
            {
                presetsFolder = candidate;
                break;
            }

            searchDir = searchDir.getParentDirectory();
        }
    }

    // Create the directory if it still doesn't exist
    if (!presetsFolder.exists())
        presetsFolder.createDirectory();

    return presetsFolder;
}

//==============================================================================
bool PresetManager::savePreset (const juce::String& name)
{
    if (name.isEmpty())
        return false;

    auto dir = getPresetsDirectory();
    if (!dir.isDirectory())
        return false;

    auto file = dir.getChildFile (name + presetExtension);

    processor.currentPresetName = name;
    auto state = processor.getStateTree();
    auto xml   = state.createXml();

    if (!xml)
        return false;

    return xml->writeTo (file);
}

//==============================================================================
bool PresetManager::loadPreset (const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    auto xml = juce::parseXML (file);

    if (!xml)
        return false;

    auto state = juce::ValueTree::fromXml (*xml);

    if (!state.isValid())
        return false;

    processor.loadFromValueTree (state);
    return true;
}

//==============================================================================
juce::Array<juce::File> PresetManager::getPresetFiles() const
{
    juce::Array<juce::File> files;
    getPresetsDirectory().findChildFiles (files, juce::File::findFiles, false,
                                          "*" + juce::String (presetExtension));

    struct NameSorter
    {
        static int compareElements (const juce::File& a, const juce::File& b)
        {
            return a.getFileName().compareNatural (b.getFileName());
        }
    };

    NameSorter sorter;
    files.sort (sorter);
    return files;
}

//==============================================================================
juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;

    for (const auto& f : getPresetFiles())
        names.add (f.getFileNameWithoutExtension());

    return names;
}
