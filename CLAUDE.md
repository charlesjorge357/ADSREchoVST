# ADSREchoVST — Developer Notes for Claude

## Project Overview

JUCE VST3 audio plugin. C++17, CMake + Projucer.  
Primary test DAW: FL Studio. Also tested in Ableton Live.  
Working directory: `c:\Users\Owner\UnityProjects\gun-oil\ADSREchoVST`

### Architecture

- `PluginProcessor` owns `slots`: 2 chains × up to 8 `ModuleSlot` (8×8 grid)
- Module types: `DelayModule`, `ReverbModule` (Dattorro/HybridPlate), `ConvolutionModule`, `EQModule`, `CompressorModule`
- `Convolution.h/cpp` wraps KlangFalter `TwoStageFFTConvolver` via `ThreadedConvolver`
- `IRBank` scans for `.wav` files next to the plugin binary at startup (index 0 = Bypass)
- `ModuleSlot` holds `ownedModule` (unique_ptr) and `activeModule` (atomic ptr for audio thread)

---

## Issue #1: "Cooked State" on Project Reload

### Symptom
Loading a project that already has ADSREcho loaded causes a broken state:
- Preset name restored correctly but 0 modules visible in editor
- CPU usage spikes
- Adding modules causes undefined behavior
- Closing FL Studio hangs

First load from empty works. Only reload (project already loaded) triggers it.

### Root Cause Analysis

**PATH A vs PATH B in `loadFromValueTree`:**
- PATH A (`spec.sampleRate == 0`): install modules immediately, synchronously
- PATH B (`spec.sampleRate > 0`): background thread prepares modules, `callAsync` swaps them in

On any reload, `prepareToPlay` has already been called → `spec.sampleRate > 0` → **every reload takes PATH B**.

**VST3 dual `setComponentState` calls:**  
VST3 hosts call `setComponentState` on **both** the audio component AND the edit controller. The controller call arrives after `prepareToPlay` with a state that has **no `Modules` child** (parameter values only). Without a guard, this second call triggers `doSwap` with an empty `incoming` vector → all installed modules are wiped.

**Hang on close:**  
The `callAsync` lambda from PATH B captures `incoming` modules (possibly ConvolutionModules with active convolver threads). When the processor is destroyed before the callAsync fires, the `weakThis == nullptr` early-return path destroys `incoming` without pre-signaling the convolver threads → sequential `stopBackgroundThread()` joins (up to 500ms each) on the message thread.

**Empty destructor:**  
`~ADSREchoAudioProcessor()` was empty. Active ConvolutionModules in `slots` had their convolver threads joined sequentially (one thread at a time, 500ms timeout each) during member destruction, blocking the message thread for N × 2 × 500ms.

### Fixes Applied (Source/PluginProcessor.cpp)

**Fix 1 — Missing Modules child guard** (near top of `loadFromValueTree`):
```cpp
auto modules = state.getChildWithName("Modules");
if (!modules.isValid())
{
    DBG("loadFromValueTree: no Modules child — APVTS-only update, slots preserved");
    currentPresetName = state.getProperty("currentPresetName", "").toString();
    apvts.replaceState(state);
    uiNeedsRebuild.store(true, std::memory_order_release);
    return;
}
```
Prevents the VST3 controller's parameter-only `setComponentState` call from wiping installed modules.

**Fix 2 — Pre-signal convolvers in `weakThis == nullptr` PATH B path:**
```cpp
if (self == nullptr)
{
    for (auto& p : incoming)
        if (auto* cm = dynamic_cast<ConvolutionModule*>(p.module.get()))
            cm->signalConvolversToStop();
    return;
}
```
Signals all convolver threads to exit in parallel before their sequential destructions.

**Fix 3 — Processor destructor pre-signals all convolvers:**
```cpp
ADSREchoAudioProcessor::~ADSREchoAudioProcessor()
{
    ++(*loadGeneration); // abort any in-flight PATH B background thread early
    for (auto& chain : slots)
        for (auto& slot : chain)
            if (auto* cm = dynamic_cast<ConvolutionModule*>(slot->get()))
                cm->signalConvolversToStop();
}
```
Allows all active convolver threads to start exiting in parallel before member destructors run their sequential joins.

### Threading Architecture Notes

- `JUCE_DECLARE_WEAK_REFERENCEABLE` is declared in `PluginProcessor.h` — the `masterReference` member is destroyed **first** (declared last = destroyed first). By the time `slots` and `apvts` are destroyed, `weakThis.get()` already returns null. Thread-safe without additional locking.
- `loadGeneration` (`shared_ptr<atomic<uint64_t>>`): incremented on every PATH B call AND in the destructor. Captured by background thread; stale threads see generation mismatch and abort.
- `loadGeneration` is declared before the WeakReference macro (line 139 vs 149), so it outlives the masterReference clear — safe for background threads to read after the processor would otherwise appear "dead" to a WeakReference.

### FL Studio Specific Rules

- Never start `juce::Thread` in a JUCE object constructor — FL crashes during plugin scan
- `waitForThreadToExit` from message thread is OK in release builds
- Auto-reset `WaitableEvent` consumed by first wait → deadlock. Use manual-reset for "done" events
- `juce::Thread::~Thread()` jasserts if thread still running — always stop threads in derived destructor

---

## Issue #2: Multi-Instance Silent Crash on Project Switch (same root cause as Issue #1)

### Symptom
Switching from a project with multiple ADSREcho instances causes a silent crash (FL Studio dies with no log output). Began when KlangFalter (ConvolutionModule) was added.

### Root Cause — `ThreadedConvolver` deadlock under audio lock

Both Issues 1 and 2 share the same underlying deadlock. With Delay/Reverb modules the prepare time is near-zero so the race window is never hit; with ConvolutionModule it is wide open.

**Exact sequence:**
1. Message thread calls `signalToStop()` → sets `threadShouldExit()=true`, signals `startEvent`, **does NOT signal `doneEvent`**
2. Background tail thread wakes, sees `threadShouldExit()=true`, skips `doBackgroundProcessing()`, exits run() **without signaling `doneEvent`** — `doneEvent` remains unset
3. Audio thread (still processing the old module under callback lock) calls `process()` → `startBackgroundProcessing()` → **`doneEvent.reset()`** (re-arms the trap), signals `startEvent` (thread already dead)
4. Audio thread calls `waitForBackgroundProcessing()` → **`doneEvent.wait(-1)` blocks forever**, holding the callback lock
5. Message thread's `doSwap()` tries `ScopedLock(getCallbackLock())` → **deadlock**
6. FL Studio watchdog fires → "cooked state" (single instance) or crash (N instances simultaneously)

### Fix Applied — `Source/Convolution.h` (ThreadedConvolver)

Four changes in `ThreadedConvolver`:

**1. `signalToStop()` — also signal `doneEvent`:**
Immediately unblocks any audio-thread `waitForBackgroundProcessing()` that fires between the signal and the thread's actual exit.

**2. `startBackgroundProcessing()` — guard against re-arming after exit (single check):**
If `threadShouldExit()`, return without calling `doneEvent.reset()`. Handles the case where exit was requested before this call.

**3. `startBackgroundProcessing()` — TOCTOU double-check after `doneEvent.reset()`:**
`signalToStop()` can fire between the first `threadShouldExit()` check and `doneEvent.reset()`, undoing signalToStop()'s `doneEvent.signal()`. After resetting doneEvent, check `threadShouldExit()` again; if now true, re-signal doneEvent before returning so `waitForBackgroundProcessing()` is never stranded on a dead thread. This is the primary fix for the remaining intermittent cooked-state on project return-switch.

```cpp
void startBackgroundProcessing() override
{
    if (threadShouldExit())
        return;
    doneEvent.reset();
    // TOCTOU guard: signalToStop() may have fired between the check above and
    // doneEvent.reset(), undoing signalToStop()'s doneEvent.signal().
    if (threadShouldExit())
    {
        doneEvent.signal();
        return;
    }
    startEvent.signal();
}
```

**4. `run()` — signal `doneEvent` unconditionally on exit:**
Covers the case where the audio thread was already blocked on `doneEvent.wait(-1)` when the thread decided to exit.

### Fix Applied — `Source/Convolution.cpp` (`Convolution::prepare()`)

When `prepareToPlay()` fires while PATH B is in flight, `prepare()` was calling `loadIRAtIndex()` on old modules even though `irLoadSuppressed = true`. This:
- Did unnecessary disk I/O + FFT on the message thread (could take seconds)
- Re-started convolver background threads on modules about to be destroyed
- Widened the TOCTOU race window significantly (longer time between `signalToStop()` and thread actually exiting)

Fix: check `irLoadSuppressed` at the TOP of `prepare()`, before `reset()` is even called. When suppressed, return immediately — the old module keeps its existing state and continues producing correct audio until `doSwap()` replaces it.

```cpp
if (irLoadSuppressed.load(std::memory_order_relaxed))
    return;  // skip reset() + IR reload; module is being replaced by PATH B

reset();
// ... normal prepare continues
```

**Why the early return must be before `reset()`:** `Convolution::reset()` calls `convolverL.waitForCompletion()` (waits for in-flight tail FFT) then `TwoStageFFTConvolver::reset()` (which calls `_backgroundProcessingInput.clear()`). Between `waitForCompletion()` returning and `TwoStageFFTConvolver::reset()` running, the audio thread can fire `startBackgroundProcessing()` and queue a NEW tail job — the background thread then reads `_backgroundProcessingInput` while the message thread is clearing it. This is a use-after-free / data race that causes non-deterministic crashes.

---

## Issue #3: DAW Prompts to Save on Plugin Open/Close

### Symptom
Both FL Studio and Ableton Live mark the project as dirty and prompt "save?" when the user merely opens or closes the ADSREcho editor. No parameters were changed.

### Root Cause (identified)

**Primary: `SliderAttachment` / `ButtonAttachment` created during editor open notify the host**

`MasterPanel::attachToAPVTS()` called from `PluginEditor.cpp:27` during editor construction creates `SliderAttachment` and `ButtonAttachment` objects. JUCE attachments call `sendInitialUpdate()` on creation, which in some JUCE versions calls `setValueNotifyingHost()` — telling the host a parameter changed even though the user did nothing.

**Call chain:**
```
createEditor()
  → PluginEditor constructor
    → masterPanels[chain]->attachToAPVTS(apvts)   [PluginEditor.cpp:27]
      → SliderAttachment / ButtonAttachment created [MasterPanel.cpp:146-157]
        → sendInitialUpdate() → setValueNotifyingHost()
          → host marks project dirty
```

Same pattern fires again in `rebuildModuleEditors()` (PluginEditor.cpp:388-478) whenever the UI rebuilds — which happens on editor open and on any module change.

**Secondary: `setValueNotifyingHost()` in `setSlotDefaults`** (PluginProcessor.cpp:949)  
Called when adding a module. Notifies host of every parameter reset. Correct behavior but worth knowing.

### Fix Direction

In `MasterPanel::attachToAPVTS()` and all panel `attachToAPVTS()` methods: wrap attachment creation so the initial value sync does **not** propagate a host change notification. Options:

1. **Check if JUCE version's attachment calls `setValueNotifyingHost` on init.** If so, temporarily suppress with `beginChangeGesture()` + set + `endChangeGesture()` workaround, or upgrade JUCE.
2. **Use `AudioProcessorValueTreeState::Listener` instead of attachments** for the initial sync, and only attach for user-driven changes.
3. **Most reliable:** after creating each attachment, call `param->endChangeGesture()` if it was left open — or check whether `MasterPanel`/module panels have an `onValueChange` lambda that calls `setValueNotifyingHost()` directly and change it to `setValue(v, dontSendNotification)` for the non-user-driven sync path.

**Files to fix:**
- `Source/MasterPanel.cpp` lines 146-157 (`attachToAPVTS`)
- `Source/CompressorPanel.cpp` lines 151-154
- `Source/ConvolutionPanel.cpp` lines 172-175
- `Source/PluginEditor.cpp` lines 388-478 (`rebuildModuleEditors`)

**Status: root cause confirmed, fix not yet implemented.**

---

## Issue #4: Preset Format Compatibility (Historical)

### FL Preset Analysis (preset "Parallel Convo + Delay")
- Format is **current** — uses `irIndex` attribute on Slot nodes, has `Modules` subtree
- `irIndex="20"` references bank IR index 20 (valid if bank has ≥ 21 entries)
- **`<PARAM id="chain_0.enabled"/>` and `<PARAM id="chain_1.enabled"/>` have no `value` attribute** — these parameters were added to the APVTS layout after this preset was saved (older plugin version). On load, JUCE sets them to 0 (disabled). Both chains disabled = silence, not a crash, but unexpected behavior. User should resave the preset with the current version.

---

## Key Files

| File | Role |
|---|---|
| `Source/PluginProcessor.cpp` | State save/restore, PATH A/B swap logic, processBlock |
| `Source/PluginProcessor.h` | Processor class, member order (affects destruction order) |
| `Source/Convolution.cpp` | IR loading, prepare/reset, loadCustomEngineIR |
| `Source/Convolution.h` | ThreadedConvolver, doneEvent (manual-reset, pre-signaled) |
| `Source/ConvolutionModule.cpp` | Thin wrapper; caches raw APVTS param pointers |
| `Source/IRBank.h` | Scans plugin bundle for .wav files at construction |
| `Source/PluginEditor.cpp` | 30Hz timer, safety-net UI rebuild, editor lifecycle |
| `Source/ModuleSlot.h` | Holds ownedModule + atomic activeModule |

## Issue #5: FL Studio Freeze When Disabling Plugin (Mixer LED / Power Circle)

### Symptom
Clicking the power LED to disable the ConvolutionModule effect in FL Studio's mixer freezes FL Studio completely (permanent hang).

### Root Cause — `flStudioDIYSpecificationEnforcementMutex` deadlock

JUCE 8's VST3 wrapper contains `FLStudioDIYSpecificationEnforcementLock` (juce_audio_plugin_client_VST3.cpp line 3761). Because FL Studio's Patcher calls `process()` before/during `setActive()` (VST3 spec violation), JUCE serializes them with `flStudioDIYSpecificationEnforcementMutex`:

- `process()` acquires the mutex for its **entire duration** (line 3684)
- `setActive(false)` (triggered by disabling the plugin) also acquires the mutex (line 2805)

Inside `process()` → `processBlock()` → `convolverL.process()` → `TwoStageFFTConvolver::process()`, every `tailBlockSize` samples the audio thread calls `waitForBackgroundProcessing()` = `doneEvent.wait(-1)`. This **blocks while holding `flStudioDIYSpecificationEnforcementMutex`**. When FL Studio then calls `setActive(false)`, it tries to acquire the same mutex → **permanent deadlock → FL Studio frozen**.

### Why KlangFalter Doesn't Have This Problem

KlangFalter uses the **synchronous** base class (`TwoStageFFTConvolver::waitForBackgroundProcessing()` is a no-op; all tail work runs inline). We use `ThreadedConvolver` which DOES block. KlangFalter also uses `max(8192, 2 * headBlockSize)` for tailBlockSize, ensuring the background thread always finishes before the next wait.

### Fix Applied — `Source/Convolution.cpp` (`Convolution::prepare()`)

Add a minimum floor of 8192 to `tailBlockSize_`:

```cpp
tailBlockSize_ = std::max((size_t)8192, headBlockSize_ * 8);
```

With tailBlockSize ≥ 8192 at 44100Hz, `waitForBackgroundProcessing()` is called at most every 8192/44100 ≈ 186ms. Any realistic IR's tail FFT completes in 10–50ms, so `waitForBackgroundProcessing()` almost always returns immediately. The deadlock window effectively disappears.

**Without the fix:** small FL Studio buffer sizes (64–128 samples) cause `headBlockSize_=128`, `tailBlockSize_=1024` → only 23ms window → tail FFT of a complex IR regularly exceeds 23ms → audio thread blocks → disable deadlock guaranteed.

---

## Convolution Engine Notes

- `USE_CUSTOM_CONVOLVER 1` → KlangFalter `TwoStageFFTConvolver` (in `Source/fftconvolver/`)
- Thread starts lazily in `ThreadedConvolver::init()` on first IR load (not in constructor)
- `doneEvent` is **manual-reset, pre-signaled** — `waitForCompletion()` is always safe
- `startBackgroundProcessing()` resets `doneEvent` BEFORE signaling start
- `tailBlockSize` floor of 8192 ensures background thread always finishes before next `waitForBackgroundProcessing()` — critical for FL Studio disable deadlock prevention
- Energy normalization: `scale = 1/sqrt(sum of squared samples L+R)`
- `TwoStageFFTConvolver::process()` requires separate input/output pointers — copy channel to `monoInBuf` first

## Convolution CPU Optimizations (June 2026)

All audio-equivalent — FFT backends verified bin-for-bin against a reference
DFT (`Source/fftconvolver/pffft/pffft_audiofft_test.cpp`, PASS on pffft and
Ooura; build command in the file header).

- **Profiler**: build with `ADSRECHO_PROFILE_CONV=1` → DBG prints
  `conv: X ms CPU (Y% of one core)` every 5 s of audio. Baseline table at top
  of `Convolution.cpp` (TBD — measure in FL with DebugView). Compiles out.
- **SSE CMAC**: `FFTCONVOLVER_USE_SSE=1` pinned project-wide (CMake + .jucer
  project-level defines). Never define per-config — `Buffer<T>` switches
  `_mm_malloc`/`new[]` inline per TU; a mismatch is an ODR/heap hazard.
- **Silence gate** (`Convolution::processBlock`): skips convolution after
  `irTailSamples + preDelay` samples of silent input flush the state to zero.
  Sits BEFORE `pushDrySamples` so the DryWetMixer push/mix pair stays
  balanced. `irTailSamples` set in `loadCustomEngineIR` / JUCE-engine paths.
- **Per-module silence gates** (`ModuleSlot::process` + virtual
  `EffectModule::getTailLengthSamples(sr)`): generic gate — while input is
  silent, keep processing until the module's reported tail (sized to ~-120 dB
  ring-out, NOT -60, so nothing audible is truncated) plus a 1/4 s re-arm pad
  has flushed; then skip process() entirely. Resume is instant and click-free
  (state is flushed to ~zero). Tail per module: Convolution = exact IR len +
  preDelay; Reverb = 2.5×T60 + preDelay; Delay = repeats-to-−120dB from
  feedback (0.95 fb → gate never engages, correct); EQ = 3 s const;
  Compressor = 4 s const (envelope must fully release before freezing, also
  lets meters settle). Base-class default 30 s for future modules.
  gateCountdown starts at 0 (fresh modules have zero state).
- **pffft FFT backend** (`Source/fftconvolver/pffft/`, vendored marton78
  mirror, BSD-like license): selected via `AUDIOFFT_PFFFT=1`; priority chain
  in `AudioFFT.cpp` is Accelerate > pffft > FFTW3 > Ooura. `pffft.c` +
  `pffft_common.c` compile as C; `PFFFT_STATIC_DEFINE` patched into vendored
  files (statically linked). `simd/*.h` must stay on disk (resolved via
  relative includes; not registered in build). macOS uses Accelerate.
- **Head block tuning hook**: `ADSRECHO_CONV_HEAD_BLOCK` (0 = auto). Pins the
  head partition size for profiling. Do NOT touch the tailBlockSize 8192
  floor (Issue #5 FL disable deadlock).
- **AVX2 CMAC (Task 4) skipped intentionally** — gated on profiler evidence
  that CMAC still dominates after SSE + pffft.
- **Loader-thread exception guards** (`PluginProcessor.cpp`): both
  `std::thread` bodies (PATH B and `requestIRChange`) wrap their work in
  try/catch — an exception escaping a `std::thread` calls `std::terminate()`
  (= the 0xc0000409 ucrtbase abort signature seen in FL crash events).
  On failure the replacement modules are discarded and old modules stay.

## Debug Output

JUCE `DBG()` → `OutputDebugString()` on Windows → **NOT in FL Studio's log folder**.  
To see plugin debug output: run **Sysinternals DebugView** as administrator, or attach VS debugger to FL Studio.

---

## Auto-Updater (Planned)

Architecture:
- Server: host `https://yoursite.com/adsr-echo/latest.json` with `{ "version": "x.y.z", "download_url": "..." }`
- Plugin: on editor open (not processor construction), fire a one-shot background thread using `juce::URL::createInputStream()` to fetch the JSON
- Throttle: check at most once per 24 hours using `juce::PropertiesFile` in user app data
- If newer: show non-modal banner in editor with `juce::URL::launchInDefaultBrowser()`
- Never show modal dialogs from plugin editors (DAW compatibility)
