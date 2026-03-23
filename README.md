# ADSREchoVST
Senior Design Project Group L01 | Multi-Reverb Effect Plugin

ADSR-Echo is a VST3/AU audio effect plugin built with JUCE featuring reverb, delay, EQ, and compressor modules with convolution reverb support.

---

## Installation (End Users)

Download the installer for your platform from the releases page and run it. No other steps required — IRs and Presets are bundled inside.

### Windows
Run `ADSREcho-Windows-Installer-<version>.exe` and follow the prompts. The plugin installs to:
```
C:\Program Files\Common Files\VST3\ADSREcho.vst3\
```

### macOS
Open `ADSREcho-macOS-Installer-<version>.pkg` and follow the prompts. Installs both VST3 and AU:
```
/Library/Audio/Plug-Ins/VST3/ADSREcho.vst3
/Library/Audio/Plug-Ins/Components/ADSREcho.component
```

### Linux
Install the `.deb` package:
```bash
sudo dpkg -i adsr-echo_<version>_amd64.deb
```
The plugin installs to:
```
/usr/lib/vst3/ADSR-Echo.vst3/
```
To uninstall:
```bash
sudo dpkg -r adsr-echo
```

---

## Building from Source (Developers)

### Prerequisites

- CMake 3.15+
- C++17 compiler
- JUCE (included as a submodule)

**Linux additional dependencies:**
```bash
sudo apt-get install libx11-dev libasound2-dev libfreetype6-dev libfontconfig1-dev \
    libgl1-mesa-dev libjack-jackd2-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libcurl4-openssl-dev
```

**macOS:** Xcode Command Line Tools
**Windows:** Visual Studio 2022 with C++ workload

### Clone and Build

```bash
git clone --recurse-submodules https://github.com/charlesjorge357/ADSREchoVST.git
cd ADSREchoVST
cmake -B build-host
cmake --build build-host --config Release -- -j$(nproc)
```

After building, the plugin is automatically copied to your system's VST3 folder and IRs/Presets are placed alongside it.

---

## Creating Installers

### Windows — Inno Setup

1. Install [Inno Setup](https://jrsoftware.org/isinfo.php)
2. Build the project in Release (see above)
3. Compile the installer script:
```
iscc /DAppVersion=0.1.0 installer\windows\installer.iss
```
Output: `release\ADSREcho-Windows-Installer-0.1.0.exe`

### macOS — .pkg

1. Build the project in Release (see above)
2. Run the packaging script from the `installer/macos/` directory:
```bash
cd installer/macos
./build_pkg.sh 0.1.0
```
Output: `release/ADSREcho-macOS-Installer-0.1.0.pkg`

Requires `pkgbuild` and `productbuild` (included with Xcode Command Line Tools).

### Linux — .deb

1. Build the project (see above)
2. Run the packaging script from the `installer/linux/` directory:
```bash
cd installer/linux
./build_deb.sh 0.1.0
```
Output: `release/adsr-echo_0.1.0_amd64.deb`

Requires `dpkg-deb`:
```bash
sudo apt-get install dpkg
```
