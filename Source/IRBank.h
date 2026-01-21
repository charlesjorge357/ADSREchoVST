// ==============================================================================
// IRBank.h - Manages impulse response files
// ==============================================================================
#pragma once

#include <JuceHeader.h>

class IRBank
{
public:
    struct IRInfo
    {
        juce::String name;
        juce::File file;
    };

    IRBank()
    {
        loadIRsFromSourceFolder();
    }

    // Get IR file at index
    juce::File getIRFile(int index) const
    {
        if (juce::isPositiveAndBelow(index, irList.size()))
            return irList[index].file;
        return juce::File();
    }

    // Get IR name at index
    juce::String getIRName(int index) const
    {
        if (juce::isPositiveAndBelow(index, irList.size()))
            return irList[index].name;
        return "No IR";
    }

    // Get number of IRs
    int getNumIRs() const 
    { 
        return (int)irList.size(); 
    }

    // Get all IR names for UI
    juce::StringArray getIRNames() const
    {
        juce::StringArray names;
        for (const auto& ir : irList)
            names.add(ir.name);
        return names;
    }

private:
    std::vector<IRInfo> irList;

    void loadIRsFromSourceFolder()
    {
        // Get the folder containing IRs relative to the executable/plugin
        // For development, this should find Source/IRs/
        auto irFolder = juce::File::getSpecialLocation(
            juce::File::currentExecutableFile)
            .getParentDirectory()
            .getChildFile("IRs");

        // If not found, try alternative locations
        if (!irFolder.exists())
        {
            // Try user application data directory
            irFolder = juce::File::getSpecialLocation(
                juce::File::userApplicationDataDirectory)
                .getChildFile("ADSREcho")
                .getChildFile("IRs");
        }

        // If still not found, try common plugin locations
        if (!irFolder.exists())
        {
            // macOS: ~/Library/Audio/Plug-Ins/VST3/ADSREcho.vst3/Contents/Resources/IRs
            // Windows: C:/Program Files/Common Files/VST3/ADSREcho.vst3/Contents/Resources/IRs
            #if JUCE_MAC
                irFolder = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                    .getChildFile("Library/Audio/Plug-Ins/Components/ADSREcho.component/Contents/Resources/IRs");
            #elif JUCE_WINDOWS
                irFolder = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
                    .getChildFile("VST3/ADSREcho.vst3/Contents/Resources/IRs");
            #endif
        }

        DBG("Looking for IRs in: " + irFolder.getFullPathName());

        if (irFolder.exists() && irFolder.isDirectory())
        {
            // Scan for .wav files
            for (const auto& file : irFolder.findChildFiles(
                juce::File::findFiles, false, "*.wav;*.WAV"))
            {
                IRInfo info;
                info.name = file.getFileNameWithoutExtension();
                info.file = file;
                irList.push_back(info);
                
                DBG("Found IR: " + info.name + " at " + file.getFullPathName());
            }
        }
        else
        {
            DBG("IR folder not found!");
        }

        // Add a bypass option at the beginning
        if (irList.empty() || irList[0].name != "Bypass")
        {
            IRInfo bypass;
            bypass.name = "Bypass (No IR)";
            // Empty file = bypass
            irList.insert(irList.begin(), bypass);
        }

        DBG("Total IRs loaded: " + juce::String(irList.size()));
    }
};