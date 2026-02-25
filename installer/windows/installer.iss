[Setup]
AppName=ADSR-Echo
AppVersion={#AppVersion}
AppPublisher=ADSR-Echo Team
DefaultDirName={commonpf}\Common Files\VST3
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
Source: "..\..\build\ADSREcho_artefacts\Release\VST3\ADSREcho.vst3\*"; DestDir: "{app}\ADSREcho.vst3"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{commonprograms}\ADSR-Echo\Uninstall ADSR-Echo"; Filename: "{uninstallexe}"
