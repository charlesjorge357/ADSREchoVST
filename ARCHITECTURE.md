# ADSR Echo Plugin Architecture - Updated

## Overview
Professional multi-effect audio plugin featuring algorithmic reverb (Hall/Plate), advanced delay with Haas effect and ping-pong, convolution reverb with custom IR loading, compression, and parametric EQ. Includes flexible routing matrix, tempo sync, and comprehensive preset management.

## Core Design Philosophy

1. **Flexible Routing**: Effects can be chained in multiple topologies, including parallel processing
2. **Reverb Blending**: Algorithmic (Hall/Plate) and convolution reverbs can be blended
3. **Tempo Sync**: Full project BPM detection with musical subdivisions
4. **Preset System**: Factory and user presets with categories and tagging
5. **Master Dry/Wet**: Global mix control preserves original signal

## Plugin Architecture

```
ADSREchoAudioProcessor
├── Effect Modules
│   ├── DelayProcessor (Advanced stereo delay)
│   ├── ReverbProcessor (Hall/Plate algorithms)
│   ├── ConvolutionProcessor (IR-based reverb)
│   ├── CompressorProcessor (Dynamics control)
│   └── EQProcessor (3-band parametric)
├── System Modules
│   ├── RoutingMatrix (Flexible effect routing)
│   └── PresetManager (Factory + user presets)
└── Tempo/BPM Sync (Host tempo detection)
```

## Effect Modules

### 1. Delay Processor (`DelayProcessor.h`)

**Purpose**: Advanced stereo delay with multiple modes and distortion

**Features**:
- **Stereo Modes**:
  - **Stereo**: Independent L/R delay times
  - **Ping-Pong**: Alternating L/R feedback for stereo width
  - **Haas Effect**: Short delay (5-40ms) for psychoacoustic stereo enhancement

- **Tempo Sync**:
  - Musical subdivisions (whole, half, quarter, eighth, sixteenth notes)
  - Dotted notes (1/2., 1/4., 1/8.)
  - Triplets (1/2T, 1/4T, 1/8T)
  - Independent L/R sync divisions

- **Distortion Path**:
  - Waveshaper for tape-like saturation
  - Adjustable distortion amount (0-100%)
  - Distortion mix control (blend clean/distorted)

- **Filtering**:
  - Low-cut filter (20-1000Hz) - removes mud from feedback
  - High-cut filter (1-20kHz) - darkens feedback for analog character

**Parameters** (13 total):
- Delay Time L/R (1-2000ms or synced)
- Feedback (0-95%)
- Mix (0-100%)
- Stereo Mode (Stereo/PingPong/Haas)
- Sync Enable (on/off)
- Sync Division L/R (11 options)
- Haas Delay Time (5-40ms)
- Low/High Cut filters
- Distortion amount & mix

**DSP Components**:
- `DelayLineWithSampleAccess` for multi-tap delays
- `juce::dsp::IIR::Filter` for feedback filtering
- `juce::dsp::WaveShaper` for distortion

---

### 2. Reverb Processor (`ReverbProcessor.h`)

**Purpose**: Algorithmic reverb with Hall and Plate models

**Models**:
- **Hall**: Datorr o Hall algorithm - lush, spacious, natural decay
- **Plate**: Plate reverb simulation - bright, dense, shorter decay

**Parameters** (6 total):
- Reverb Type (Hall/Plate)
- Size (0-100%) - room/plate dimensions
- Diffusion (0-100%) - echo density, smoothness
- Decay (0.1-10s) - tail length
- Mix (0-100%) - dry/wet
- Pre-Delay (0-100ms) - initial gap before reverb

**DSP Components**:
- `DatorroHall<float>` (stereo) for hall reverb
- `juce::dsp::Reverb` with custom topology for plate
- Multiple `Allpass` diffusers for plate character
- `juce::dsp::DelayLine` for pre-delay

---

### 3. Convolution Processor (`ConvolutionProcessor.h`)

**Purpose**: Convolution reverb for realistic acoustic spaces

**Features**:
- **Custom IR Loading**: Load any WAV/AIFF impulse response
- **Factory IR Bank**: Pre-installed impulse responses:
  - Concert halls
  - Studios
  - Churches
  - Plates/springs
  - Creative spaces
- **IR Time Stretching**: 0.5-2.0x stretch factor for unique effects

**Parameters** (5 total):
- Enabled (on/off)
- IR Selection (preset index or custom)
- Mix (0-100%)
- Pre-Delay (0-100ms)
- IR Stretch (0.5-2.0x)

**DSP Components**:
- `juce::dsp::Convolution` (stereo)
- `juce::dsp::DelayLine` for pre-delay
- IR file manager

---

### 4. Compressor Processor (`CompressorProcessor.h`)

**Purpose**: Dynamic range compression with metering

**Parameters** (7 total):
- Enabled (on/off)
- Threshold (-60 to 0 dB)
- Ratio (1:1 to 20:1)
- Attack (0.1-100ms)
- Release (10-1000ms)
- Knee (0-12dB) - soft/hard knee
- Makeup Gain (0-24dB)

**Metering**:
- Gain reduction (real-time)
- Input level
- Output level

**DSP Components**:
- `juce::dsp::Compressor<float>`
- `juce::dsp::BallisticsFilter` for envelope detection

---

### 5. EQ Processor (`EQProcessor.h`)

**Purpose**: 3-band parametric equalizer

**Bands**:
- **Low Shelf**: 20-500Hz, ±12dB
- **Mid Peak/Notch**: 200-5000Hz, ±12dB, Q 0.1-10
- **High Shelf**: 2-20kHz, ±12dB

**Parameters** (9 total):
- Enabled (on/off)
- Low Freq, Low Gain
- Mid Freq, Mid Gain, Mid Q
- High Freq, High Gain

**DSP Components**:
- `juce::dsp::ProcessorChain` with 3 IIR filters
- Frequency response calculation for GUI visualization

---

## Routing Matrix System (`RoutingMatrix.h`)

**Purpose**: Flexible effect chain routing and parallel processing

**Pre-defined Topologies**:

1. **Classic**:
   ```
   Input → EQ → Compressor → Delay → Alg.Reverb → Conv.Reverb → Output
   ```

2. **Reverb Blend** (Parallel reverbs):
   ```
   Input → Delay → ┬→ Alg.Reverb ┬→ EQ → Output
                   └→ Conv.Reverb ┘
   ```

3. **Delay to Convolution**:
   ```
   Input → EQ → Comp → Delay → Conv.Reverb → Alg.Reverb → Output
   ```

4. **Convolution First**:
   ```
   Input → Conv.Reverb → Delay → Alg.Reverb → EQ → Comp → Output
   ```

5. **Parallel Processing**:
   ```
   Input → ┬→ Delay ──────┐
           ├→ Alg.Reverb ─┤
           ├→ Conv.Reverb ┼→ Mix → Output
           ├→ Compressor ─┤
           └→ EQ ─────────┘
   ```

6. **Custom**: User-defined routing

**Features**:
- Per-effect enable/bypass
- Send amounts for partial routing
- Reverb blend ratio control
- Serialization for presets

---

## Preset Management System (`PresetManager.h`)

**Purpose**: Comprehensive preset system for workflow efficiency

**Preset Structure**:
```cpp
struct Preset {
    String name;
    String category;
    String author;
    String description;
    ValueTree state;           // Complete plugin state
    bool isFactory;
    StringArray tags;          // For search
    Time dateCreated/Modified;
    int version;
}
```

**Features**:
- **Factory Presets**: Curated presets by developers
- **User Presets**: Save/load custom configurations
- **Categories**: Organize by type (Vocal, Drums, Ambient, etc.)
- **Tags**: Search by keywords
- **Navigation**: Next/previous preset browsing
- **Import/Export**: Share presets as `.adsrpreset` files
- **Modified Tracking**: Know when preset has been tweaked

**File Locations**:
- Factory: `<plugin_resources>/Presets/`
- User: `<user_documents>/ADSR Echo/Presets/`

---

## Tempo/BPM Synchronization

**BPM Detection**:
- Automatically reads host project tempo
- Updates delay times in real-time
- Fallback to 120 BPM if host doesn't provide tempo

**Musical Subdivisions** (for tempo-synced delay):
- Whole (1/1)
- Half (1/2)
- Quarter (1/4)
- Eighth (1/8)
- Sixteenth (1/16)
- Dotted: 1/2., 1/4., 1/8.
- Triplets: 1/2T, 1/4T, 1/8T

**Implementation**:
```cpp
// In processBlock():
if (auto* playHead = getPlayHead())
{
    if (auto positionInfo = playHead->getPosition())
    {
        if (auto bpm = positionInfo->getBpm())
        {
            currentTempo = *bpm;
            delayProcessor->setTempo(currentTempo);
        }
    }
}
```

---

## Signal Flow

### Standard Serial Processing
```
Input Audio
    ↓
[Store Dry Signal]
    ↓
[Get BPM from Host]
    ↓
┌─────────────────────────────┐
│  Effect Chain               │
│  (Determined by Routing)    │
│                             │
│  Example (Classic):         │
│  1. EQ                      │
│  2. Compressor              │
│  3. Delay                   │
│  4. Algorithmic Reverb      │
│  5. Convolution Reverb      │
└─────────────────────────────┘
    ↓
[Global Dry/Wet Mix]
    ↓
[Output Gain]
    ↓
Output Audio
```

### Parallel Reverb Blend
```
Input → Delay → Wet Buffer ─┬→ Alg.Reverb ──┬→ Blend ──→ EQ → Output
                            │                │
                            └→ Conv.Reverb ─┘
                               (Mix ratio: reverbBlendRatio)
```

---

## Complete Parameter List

**Total**: ~50 parameters

### Delay (13 params)
- delayTimeLeft, delayTimeRight
- delayFeedback, delayMix
- delaySyncEnabled
- delaySyncDivisionLeft, delaySyncDivisionRight
- delayStereoMode (Stereo/PingPong/Haas)
- delayHaasTime
- delayLowCut, delayHighCut
- delayDistortion, delayDistortionMix

### Reverb (6 params)
- reverbType (Hall/Plate)
- reverbSize, reverbDiffusion, reverbDecay
- reverbMix, reverbPreDelay

### Convolution (5 params)
- convolutionEnabled
- convolutionIRIndex (or custom path)
- convolutionMix, convolutionPreDelay
- convolutionStretch

### Compressor (7 params)
- compressorEnabled
- compressorThreshold, compressorRatio
- compressorAttack, compressorRelease
- compressorKnee, compressorMakeup

### EQ (9 params)
- eqEnabled
- eqLowFreq, eqLowGain
- eqMidFreq, eqMidGain, eqMidQ
- eqHighFreq, eqHighGain

### Global (3 params)
- masterBypass
- outputGain
- dryWetMix

### Routing (3 params)
- routingTopology
- reverbBlendRatio
- effectOrderCustom (serialized)

---

## Key Implementation Files

| File | Purpose |
|------|---------|
| `ParameterIDs.h` | All parameter ID definitions |
| `PluginProcessor.h/cpp` | Main processor, tempo sync, routing |
| `DelayProcessor.h` | Advanced delay module |
| `ReverbProcessor.h` | Hall/Plate reverb module |
| `ConvolutionProcessor.h` | Convolution reverb |
| `CompressorProcessor.h` | Compressor module |
| `EQProcessor.h` | EQ module |
| `RoutingMatrix.h` | Flexible routing system |
| `PresetManager.h` | Preset management |
| `CustomDelays.h/cpp` | Multi-tap delay utilities |
| `DatorroHall.h/cpp` | Datorr o reverb algorithm |
| `LFO.h/cpp` | Modulation (for future) |
| `ARCHITECTURE_UPDATED.md` | This file |
| `IMPLEMENTATION_ROADMAP.md` | Implementation plan |

---

## Implementation Status

### ✅ Completed (Architecture Phase)
- [x] All parameter definitions (50 params)
- [x] All effect module headers
- [x] Routing matrix system
- [x] Preset management system
- [x] Tempo/BPM sync framework
- [x] PluginProcessor integration
- [x] Signal chain routing structure

### ⏳ Pending Implementation (DSP Phase)
- [ ] DelayProcessor.cpp implementation
- [ ] ReverbProcessor.cpp (Hall + Plate algorithms)
- [ ] ConvolutionProcessor.cpp
- [ ] CompressorProcessor.cpp
- [ ] EQProcessor.cpp
- [ ] RoutingMatrix.cpp (topology logic)
- [ ] PresetManager.cpp (file I/O)
- [ ] Parameter connections in `updateAllParameters()`
- [ ] Custom GUI

---

## Unique Features Summary

1. **Reverb Blending**: Blend algorithmic and sampled reverbs for unique spaces
2. **Haas Effect**: Psychoacoustic stereo enhancement in delay
3. **Ping-Pong Delay**: Auto alternating L/R for width
4. **Tempo Sync**: 11 musical subdivisions including dotted & triplets
5. **Delay Distortion**: Built-in tape saturation
6. **Flexible Routing**: 5+ pre-defined topologies + custom
7. **Parallel Processing**: Effects can run in parallel
8. **Comprehensive Presets**: Factory + user with search/tags
9. **IR Stretching**: Time-stretch impulse responses for creative effects
10. **Master Dry/Wet**: Always preserve original signal

---

## Next Steps

See `IMPLEMENTATION_ROADMAP.md` for detailed implementation plan.

**Priority**:
1. Implement DelayProcessor.cpp (core delay DSP)
2. Implement ReverbProcessor.cpp (Hall algorithm first)
3. Test delay → reverb chain
4. Implement routing matrix logic
5. Connect all parameters in updateAllParameters()
6. Build custom GUI

---

## Build System

- **CMake**: Cross-platform builds
- **CI/CD**: GitHub Actions (Windows/macOS/Linux)
- **Formats**: VST3, AU (macOS), AAX (with SDK)
- **Testing**: Catch2 unit tests

**Current Build Status**: ✅ All platforms passing (with JUCE Xcode 15+ patch)
