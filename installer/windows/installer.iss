; Auto-detect build output: CMake or Visual Studio
#define CMakeBuild AddBackslash(SourcePath) + "..\..\build-host\ADSREcho_artefacts\Release\VST3\ADSR-Echo.vst3\Contents\Resources\moduleinfo.json"
#define VSBuild    AddBackslash(SourcePath) + "..\..\Builds\VisualStudio2022\x64\Release\VST3\ADSREcho.vst3\Contents\Resources\moduleinfo.json"

#if FileExists(CMakeBuild)
  #define VST3Source "..\..\build-host\ADSREcho_artefacts\Release\VST3\ADSR-Echo.vst3"
#elif FileExists(VSBuild)
  #define VST3Source "..\..\Builds\VisualStudio2022\x64\Release\VST3\ADSREcho.vst3"
#else
  #error "No VST3 build found. Build the project first with CMake or Visual Studio."
#endif

[Setup]
AppName=ADSR-Echo
AppVersion={#AppVersion}
AppPublisher=ADSR-Echo Team
DefaultDirName={commonpf}\Common Files\VST3
PrivilegesRequiredOverridesAllowed=dialog
DirExistsWarning=no
DisableProgramGroupPage=yes
OutputBaseFilename=ADSREcho-Windows-Installer-{#AppVersion}
OutputDir=..\..\release
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
Uninstallable=yes
UninstallDisplayName=ADSR-Echo VST3 Plugin

[Files]
Source: "{#VST3Source}\*"; DestDir: "{app}\ADSR-Echo.vst3"; Flags: recursesubdirs createallsubdirs
Source: "..\..\Source\IRs\*"; DestDir: "{app}\ADSR-Echo.vst3\Contents\x86_64-win\IRs"; Flags: recursesubdirs createallsubdirs
Source: "..\..\Source\Presets\*"; DestDir: "{app}\ADSR-Echo.vst3\Contents\x86_64-win\Presets"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{commonprograms}\ADSR-Echo\Uninstall ADSR-Echo"; Filename: "{uninstallexe}"
