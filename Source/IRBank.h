// ==============================================================================
// IRBank.h - Manages impulse response files
// Looks for IRs next to the plugin binary (where post-build script copies them)
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
        DBG("=== IRBank Constructor ===");
        loadIRsFromPluginBundle();
        DBG("IRBank initialized with " + juce::String(irList.size()) + " total entries");
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
    
    void loadIRsFromPluginBundle()
    {
        DBG("=== IRBank::loadIRsFromPluginBundle ===");
        
        // Get the plugin binary location
        auto pluginPath = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        DBG("Plugin Path: " + pluginPath.getFullPathName());
        
        juce::File irFolder;
        
        #if JUCE_WINDOWS
            // Windows VST3 structure:
            // ADSREcho.vst3/Contents/x86_64-win/ADSREcho.vst3
            // We want: ADSREcho.vst3/Contents/x86_64-win/IRs
            irFolder = pluginPath.getParentDirectory().getChildFile("IRs");
            DBG("Windows: Looking in plugin folder");
        #elif JUCE_MAC
            // macOS VST3 structure:
            // ADSREcho.vst3/Contents/MacOS/ADSREcho
            // We want: ADSREcho.vst3/Contents/Resources/IRs
            irFolder = pluginPath.getParentDirectory()
                                 .getParentDirectory()
                                 .getChildFile("Resources")
                                 .getChildFile("IRs");
            DBG("macOS: Looking in Resources folder");
        #else
            // Linux fallback
            irFolder = pluginPath.getParentDirectory().getChildFile("IRs");
            DBG("Linux: Looking in plugin folder");
        #endif
        
        DBG("IR Folder: " + irFolder.getFullPathName());
        DBG("Exists: " + juce::String(irFolder.exists() ? "YES" : "NO"));
        DBG("Is Directory: " + juce::String(irFolder.isDirectory() ? "YES" : "NO"));
        
        // If not found in plugin bundle, try development location
        if (!irFolder.exists())
        {
            DBG("Not found in plugin bundle, trying development location...");
            
            // Try to find Source/IRs by going up from plugin
            auto searchDir = pluginPath.getParentDirectory();
            
            for (int i = 0; i < 6; ++i)
            {
                auto testFolder = searchDir.getChildFile("Source").getChildFile("IRs");
                DBG("  Trying: " + testFolder.getFullPathName());
                
                if (testFolder.exists() && testFolder.isDirectory())
                {
                    irFolder = testFolder;
                    DBG("  Found development IRs!");
                    break;
                }
                
                searchDir = searchDir.getParentDirectory();
            }
        }
        
        if (!irFolder.exists() || !irFolder.isDirectory())
        {
            DBG("X IR folder not found anywhere!");
            DBG("  Expected location: " + irFolder.getFullPathName());
            DBG("  Please run post-build script to copy IRs, or manually create this folder and add .wav files");
            
            // Add bypass only
            IRInfo bypass;
            bypass.name = "Bypass";
            bypass.file = juce::File();
            irList.push_back(bypass);
            return;
        }
        
        // Found the folder, scan for WAV files
        DBG("Scanning for .wav files...");
        
        // Add bypass first
        {
            IRInfo bypass;
            bypass.name = "Bypass";
            bypass.file = juce::File();
            irList.push_back(bypass);
            DBG("  [0] Bypass");
        }
        
        // Get all WAV files
        juce::Array<juce::File> wavFiles;
        irFolder.findChildFiles(wavFiles, 
                               juce::File::findFiles, 
                               false,  // not recursive
                               "*.wav;*.WAV");
        
        // Sort alphabetically
        struct FileSorter
        {
            static int compareElements(const juce::File& first, const juce::File& second)
            {
                return first.getFileName().compareNatural(second.getFileName());
            }
        };
        
        FileSorter sorter;
        wavFiles.sort(sorter);
        
        DBG("Found " + juce::String(wavFiles.size()) + " WAV files");
        
        // Add each WAV file
        for (const auto& file : wavFiles)
        {
            IRInfo info;
            info.name = file.getFileNameWithoutExtension();
            info.file = file;
            irList.push_back(info);
            
            DBG("  [" + juce::String(irList.size()-1) + "] " + info.name);
            DBG("      " + file.getFullPathName());
        }
        
        DBG("Total IRs loaded: " + juce::String(irList.size()));
        DBG("=============================");
    }
};