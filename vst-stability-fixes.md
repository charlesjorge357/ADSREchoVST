# ADSREcho VST — Stability Fix Implementation Spec

You are working on a JUCE audio plugin (ADSREcho) with a modular effect-chain
architecture and a custom convolution reverb built on HiFi-LoFi's FFTConvolver
(KlangFalter-style `ThreadedConvolver`). The plugin crashes and shows runaway
memory when FL Studio restores a saved project, and can crash in-session when
IRs are loaded while audio is running.

Root causes (already diagnosed — do not re-investigate, implement the fixes):

1. `loadFromValueTree()` PATH B spawns a **detached** `std::thread` that is never
   joined. The processor (and the plugin binary) can be destroyed while it runs
   → use-after-free / code-after-unload.
2. The legacy-state fallback reads `state.getProperty(paramKey)` but APVTS
   stores values as `<PARAM id=... value=.../>` **children**, not root
   properties → always returns 0 → wrong IR pre-loaded → first audio block
   does a full disk-I/O + FFT IR load **on the audio thread** → FL watchdog.
3. IR loading can run concurrently with `process()` on the same convolver:
   - `ConvolutionPanel` Browse button calls `mod->loadCustomIR(file)` on the
     message thread against a **live** module. `FFTConvolver::reset()` raw-
     `delete`s the `_segments[]` arrays the audio thread is iterating → UAF.
   - `ConvolutionPanel` dropdown / host automation changes `convIrIndex`, and
     `Convolution::setParameters()` calls `loadIRAtIndex()` **inside the audio
     callback** (disk I/O, resampling, FFTs, `startThread()`).
4. No cap on IR length: `readStereoIR` truncates `int64 lengthInSamples` to
   `int` and allocates unbounded buffers; `FFTConvolver::init()` is not
   exception-safe (`_segCount` set before vectors filled; `bad_alloc` mid-loop
   poisons cleanup).
5. Misc: PATH A swap may run off the message thread; `ConvolutionPanel`
   callbacks capture stale `chainIdx`/`slotIdx` across slot reorders;
   `getTailLengthSeconds()` returns 0; `FFTCONVOLVER_USE_SSE` is auto-detected
   per-TU (ODR hazard if flags ever differ).

Hard constraints — these are real-time audio rules, never violate them:

- The audio thread (`processBlock` and anything it calls) must never allocate,
  do file I/O, take locks that a loading thread holds for long, start threads,
  or destroy `ConvolutionModule`s (their destructors join threads).
- Old modules must keep producing audio until fully-prepared replacements are
  swapped in under `getCallbackLock()` (the existing PATH B pattern — keep it).
- `setStateInformation` must return promptly (FL Studio times it out).
- Do not change the saved-state format; new states must remain loadable by the
  shipped version where practical, and old states must load correctly.

Apply the tasks in order. After each task the project must still compile.

---

## Task 1 — Own and join the PATH B loader thread

**Files:** `PluginProcessor.h`, `PluginProcessor.cpp`

### 1a. Add members to `ADSREchoAudioProcessor` (private):

```cpp
// Background loader for PATH B / async IR changes. Owned and joined — a
// detached thread can outlive the processor and even the plugin binary.
std::thread loaderThread;
std::mutex  loaderMutex;   // guards loaderThread join/replace

void joinLoaderThread()
{
    std::lock_guard<std::mutex> lg (loaderMutex);
    if (loaderThread.joinable())
        loaderThread.join();
}
```

`#include <mutex>` if not already present.

### 1b. In `loadFromValueTree()` PATH B, replace

```cpp
std::thread([...]{ ... }).detach();
```

with

```cpp
{
    std::lock_guard<std::mutex> lg (loaderMutex);
    if (loaderThread.joinable())
        loaderThread.join();   // fast: generation bump makes the old one abort
    loaderThread = std::thread([...]{ /* same body */ });
}
```

Keep the existing lambda body, generation checks, and the nested
`MessageManager::callAsync` swap exactly as they are.

### 1c. Destructor ordering — make this the FIRST thing in
`~ADSREchoAudioProcessor()`:

```cpp
ADSREchoAudioProcessor::~ADSREchoAudioProcessor()
{
    ++(*loadGeneration);   // tell any in-flight load to abort between modules
    joinLoaderThread();    // MUST complete before apvts/slots are destroyed

    // existing pre-signal loop stays, but extend it to cover pendingDeletion
    // modules too (see Task 1d)
    ...
}
```

### 1d. Pre-signal *all* owned modules, not just active ones. The current loop
uses `slot->get()` which misses `pendingDeletion`. Add to `ModuleSlot`:

```cpp
// Signal convolver threads in every module this slot owns (active + pending).
template <typename Fn>
void forEachOwnedModule (Fn&& fn)
{
    if (ownedModule)     fn (ownedModule.get());
    if (pendingDeletion) fn (pendingDeletion.get());
}
```

and in the destructor:

```cpp
for (auto& chain : slots)
    for (auto& slot : chain)
        slot->forEachOwnedModule ([] (EffectModule* m)
        {
            if (auto* cm = dynamic_cast<ConvolutionModule*> (m))
                cm->signalConvolversToStop();
        });
```

### 1e. Inside the loader-thread lambda, also check the generation inside the
per-module work where cheap (you already check between `prepare()` and
`forceReloadIR()` — keep that; no further granularity required).

**Acceptance:** destroying the processor while a preset is mid-load neither
crashes nor leaks; ThreadSanitizer (if available) reports no race between the
loader and the destructor.

---

## Task 2 — Fix the legacy `convIrIndex` fallback (PARAM children, not properties)

**File:** `PluginProcessor.cpp`, in `loadFromValueTree()` Phase 1, the
`else` branch where `irIdxFromXml < 0`.

Replace:

```cpp
auto paramKey = slots[chainIndex][slotIndex]->slotID + ".convIrIndex";
pendingIRIndex = (int)(float) state.getProperty(paramKey, 0.0f);
```

with:

```cpp
// APVTS serialises parameter values as <PARAM id="..." value="..."/> children
// of the root tree — NOT as root properties. getProperty(paramKey) always
// returned the default here, which made PATH B pre-load the wrong IR and
// pushed the real load onto the audio thread after the swap.
auto paramKey = slots[chainIndex][slotIndex]->slotID + ".convIrIndex";
pendingIRIndex = 0;
for (const auto& child : state)
{
    if (child.hasType ("PARAM") && child["id"].toString() == paramKey)
    {
        pendingIRIndex = (int) (float) child.getProperty ("value", 0.0f);
        break;
    }
}
```

**Acceptance:** load a state XML that has a Convolution slot **without** the
`irIndex` slot property but with `<PARAM id="chain_0.slot_0.convIrIndex"
value="5.0"/>` — after restore, `Convolution::currentIRIndex == 5` before the
first `processBlock`, and no `loadIRAtIndex` runs on the audio thread (add a
temporary `jassert(! isAudioThread)` in `loadIRAtIndex` while testing; see
Task 3 which makes this structural).

---

## Task 3 — Eliminate ALL audio-thread IR loading (single async load path)

This is the core refactor. Strategy: the audio thread only ever *requests* an
IR change; an owned background path builds a fully-prepared replacement module
and swaps it in under the callback lock — the same pattern PATH B already uses.

### 3a. `Convolution` — replace the audio-thread load with a request flag

**File:** `Convolution.h` / `Convolution.cpp`

Add:

```cpp
// Set by the audio thread when convIrIndex changes; consumed by the message
// thread. -1 = no request pending.
std::atomic<int> requestedIRIndex { -1 };

int  consumeIRRequest()        { return requestedIRIndex.exchange (-1, std::memory_order_acq_rel); }
bool hasPendingIRRequest() const { return requestedIRIndex.load (std::memory_order_acquire) >= 0; }
```

In `Convolution::setParameters()`, replace:

```cpp
if (irChanged && !customIRActive && prepared && !irLoadSuppressed.load(...))
    loadIRAtIndex(newParams.irIndex);
```

with:

```cpp
if (irChanged && ! customIRActive && prepared
    && ! irLoadSuppressed.load (std::memory_order_relaxed))
{
    // NEVER load on the audio thread: disk I/O + FFT + thread start here is
    // what trips FL Studio's watchdog. Post a request instead; the processor
    // polls it on the message thread and swaps in a prepared replacement.
    requestedIRIndex.store (newParams.irIndex, std::memory_order_release);
}
```

`loadIRAtIndex` / `forceLoadIRAtIndex` / `loadCustomIR` remain, but after this
task they are only ever called from `prepareToPlay`, the loader thread, or
other message-thread contexts on modules that are NOT installed in a slot
(except `prepareToPlay`'s pre-load, which the host serialises against
`processBlock` — that one is allowed).

Expose the request on the module:

```cpp
// ConvolutionModule.h / .cpp
int  consumeIRRequest()          { return convolutionReverb.consumeIRRequest(); }
bool hasPendingIRRequest() const { return convolutionReverb.hasPendingIRRequest(); }
```

### 3b. `ADSREchoAudioProcessor` — single entry point for IR changes

**File:** `PluginProcessor.h` / `PluginProcessor.cpp`

```cpp
public:
    // The ONLY way to change a convolution IR at runtime. Builds a fully
    // prepared replacement module on the loader thread, then swaps it in
    // under the callback lock. Safe to call from the message thread only.
    // bankIndex >= 0 → bank IR; customFile valid → custom IR file.
    void requestIRChange (const juce::String& slotID,
                          int bankIndex,
                          const juce::File& customFile = {});
```

Implementation sketch:

```cpp
void ADSREchoAudioProcessor::requestIRChange (const juce::String& slotID,
                                              int bankIndex,
                                              const juce::File& customFile)
{
    jassert (juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Resolve slot by ID — indices go stale across slot moves.
    int chainIndex = -1, slotIndex = -1;
    for (int c = 0; c < NUM_CHAINS && chainIndex < 0; ++c)
        for (int s = 0; s < MAX_SLOTS; ++s)
            if (slots[c][s]->slotID == slotID)
                { chainIndex = c; slotIndex = s; break; }
    if (chainIndex < 0) return;
    if (dynamic_cast<ConvolutionModule*> (slots[chainIndex][slotIndex]->get()) == nullptr)
        return;

    auto replacement = std::make_unique<ConvolutionModule> ("null", apvts);
    replacement->setIRBank (irBank);
    replacement->setID (slots[chainIndex][slotIndex]->slotID);
    if (customFile.existsAsFile())
        replacement->setCustomIRPathDeferred (customFile);

    const auto capturedSpec  = spec;
    const auto myGeneration  = ++(*loadGeneration);
    auto genPtr              = loadGeneration;
    juce::WeakReference<ADSREchoAudioProcessor> weakThis (this);

    {
        std::lock_guard<std::mutex> lg (loaderMutex);
        if (loaderThread.joinable()) loaderThread.join();
        loaderThread = std::thread (
            [weakThis, genPtr, capturedSpec, myGeneration,
             chainIndex, slotIndex, bankIndex,
             mod = std::move (replacement)]() mutable
        {
            if (capturedSpec.sampleRate > 0)
                mod->prepare (capturedSpec);

            if (genPtr->load (std::memory_order_acquire) == myGeneration
                && bankIndex >= 0)
                mod->forceReloadIR (bankIndex);

            juce::MessageManager::callAsync (
                [weakThis, genPtr, myGeneration,
                 chainIndex, slotIndex, mod = std::move (mod)]() mutable
            {
                auto* self = weakThis.get();
                if (self == nullptr
                    || myGeneration != genPtr->load (std::memory_order_acquire))
                {
                    mod->signalConvolversToStop();   // dtor joins, fast
                    return;
                }
                {
                    const juce::ScopedLock sl (self->getCallbackLock());
                    self->slots[chainIndex][slotIndex]
                        ->installPreparedModule (std::move (mod));
                }
                // Old module now sits in pendingDeletion. Retire it off-lock:
                self->slots[chainIndex][slotIndex]->retirePending();
                self->uiNeedsRebuild.store (true, std::memory_order_release);
            });
        });
    }
}
```

Add to `ModuleSlot`:

```cpp
// Destroy pendingDeletion on the calling (message) thread, signalling its
// convolver threads first so the join is fast. Never call under the audio lock.
void retirePending()
{
    if (auto* cm = dynamic_cast<ConvolutionModule*> (pendingDeletion.get()))
        cm->signalConvolversToStop();
    pendingDeletion.reset();
}
```

(`#include "ConvolutionModule.h"` in whatever TU needs the cast, or take a
callback — keep it simple and include it.)

NOTE: `installPreparedModule` is called for a slot whose old module is still
active; `pendingDeletion` keeps it alive through any in-flight `process()`
call, exactly like the existing design. `retirePending()` runs after the lock
is released, on the message thread, so the destructor's thread-join cannot
stall audio.

### 3c. Poll the request flag on the message thread

The processor needs a poller that works even with the editor closed. Make the
processor a `juce::Timer` (or `private juce::Timer` member class) started in
the constructor at ~10 Hz:

```cpp
// PluginProcessor.h
class ADSREchoAudioProcessor : public juce::AudioProcessor,
                               public juce::ChangeBroadcaster,
                               private juce::Timer
{
    ...
    void timerCallback() override;
};

// PluginProcessor.cpp — constructor:
startTimerHz (10);

// destructor (after joinLoaderThread()):
stopTimer();

void ADSREchoAudioProcessor::timerCallback()
{
    for (auto& chain : slots)
        for (auto& slot : chain)
            if (auto* cm = dynamic_cast<ConvolutionModule*> (slot->get()))
                if (cm->hasPendingIRRequest())
                {
                    const int idx = cm->consumeIRRequest();
                    if (idx >= 0)
                        requestIRChange (slot->slotID, idx);
                }
}
```

(One request can supersede another via the generation counter — that is the
desired behaviour when a user scrubs the IR knob.)

### 3d. Guard against regressions

Add to `Convolution::loadIRAtIndex` and `Convolution::loadIR` (debug only):

```cpp
// IR loading must never run on the audio thread.
jassert (! juce::MessageManager::existsAndIsCurrentThread()
         || true /* message thread ok */);
// Better: assert we are NOT inside an audio callback. JUCE has no direct
// query; instead assert the module is not currently installed/active, or
// simply leave a comment + rely on the structural change. At minimum:
JUCE_ASSERT_MESSAGE_THREAD_OR_NOT_AUDIO; // if unavailable, omit
```

If no clean assertion exists, skip the assert — the structural change in 3a is
the real protection.

**Acceptance:** automate `convIrIndex` from the host during playback — audio
never glitches/blocks, the IR changes within ~100–200 ms, and no allocation or
file I/O occurs in `processBlock` (verify by code inspection of the remaining
call graph from `processBlock`).

---

## Task 4 — Fix `ConvolutionPanel` (browse + dropdown + stale indices)

**Files:** `ConvolutionPanel.h`, `ConvolutionPanel.cpp`

### 4a. Store the slot identity by ID, not indices:

```cpp
// header: replace
int chainIdx = -1; int slotIdx = -1;
// with
juce::String boundSlotID;
```

Set `boundSlotID = slotID;` in `attachToAPVTS`.

### 4b. Browse button — never touch the live module:

```cpp
fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
{
    const auto file = fc.getResult();
    if (! file.existsAsFile() || proc == nullptr)
        return;

    // Builds a prepared replacement off-thread and swaps it in safely.
    proc->requestIRChange (boundSlotID, /*bankIndex*/ -1, file);

    dropDown.setSelectedId (0, juce::dontSendNotification);
    dropDown.setText (file.getFileNameWithoutExtension(),
                      juce::dontSendNotification);
});
```

Delete the `mod->loadCustomIR(file)` call. (`ConvolutionModule::loadCustomIR`
can stay for now; nothing should call it on an installed module.)

### 4c. Dropdown `onChange` — keep the parameter gesture, drop the direct
module mutation:

```cpp
dropDown.onChange = [this, &apvts, slotID]
{
    const int idx = dropDown.getSelectedId() - 1;
    if (idx < 0) return; // setText/custom-IR display, not a real selection

    if (auto* p = apvts.getParameter (slotID + ".convIrIndex"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
        p->endChangeGesture();
    }

    // Selecting a bank IR replaces any custom IR via the same safe path.
    if (proc != nullptr)
        proc->requestIRChange (boundSlotID, idx);

    irMissingLabel.setVisible (false);
};
```

Remove the `mod->clearCustomIR()` call (the replacement module simply has no
custom IR). Note `requestIRChange` makes the parameter-driven audio-thread
request flag from Task 3c redundant for UI changes — that's fine; the
generation counter de-duplicates, and the flag still covers host automation.

`#include "PluginProcessor.h"` is already present via the header.

**Acceptance:** clicking Browse and selecting a 30 s IR during playback causes
no dropout and no crash; reordering slots while the file dialog is open and
then picking a file loads the IR into the *original* module's slot (or no-ops
if it was removed) — never a different module.

---

## Task 5 — Cap IR size and harden the decode path

**File:** `Convolution.cpp` (and `Convolution.h` for the constant)

### 5a. Add a hard cap:

```cpp
// Convolution.h, alongside the other constants:
static constexpr juce::int64 kMaxIRSeconds = 20;   // generous for halls
```

### 5b. In BOTH `readStereoIR` and `readStereoIRFromMemory`, replace the
unchecked read with:

```cpp
const juce::int64 rawLen = reader->lengthInSamples;
const double fileSampleRate = reader->sampleRate;
if (rawLen <= 0 || fileSampleRate <= 0)
    return {};

const juce::int64 maxLen =
    (juce::int64) (fileSampleRate * (double) kMaxIRSeconds);
const int numSamples = (int) std::min (rawLen, maxLen);   // truncate, don't reject
const int numCh      = (int) reader->numChannels;
if (numCh <= 0)
    return {};
```

(Reading only `numSamples` from the reader is already what the existing
`reader->read (&buf, 0, numSamples, 0, true, true)` does once `numSamples` is
clamped.)

If `rawLen > maxLen`, `DBG` a warning that the IR was truncated to
`kMaxIRSeconds` seconds.

### 5c. Defense in depth around the convolver init (OOM must not poison
state). In `loadCustomEngineIR`, wrap the two `init` calls:

```cpp
bool okL = false, okR = false;
try
{
    okL = convolverL.init (headBlockSize_, tailBlockSize_, dataL, lenL);
    okR = convolverR.init (headBlockSize_, tailBlockSize_, dataR, lenR);
}
catch (const std::bad_alloc&)
{
    DBG ("Convolution: IR too large to allocate — falling back to bypass");
}
if (! okL || ! okR)
{
    std::vector<float> impulse (1, 1.0f);
    convolverL.init (headBlockSize_, tailBlockSize_, impulse.data(), 1);
    convolverR.init (headBlockSize_, tailBlockSize_, impulse.data(), 1);
}
```

### 5d. Make `FFTConvolver` cleanup safe after a partial init.
**File:** `fftconvolver/FFTConvolver.cpp` — change `reset()` to iterate the
vectors instead of `_segCount` (this is the minimal upstream-friendly patch):

```cpp
void FFTConvolver::reset()
{
  for (size_t i = 0; i < _segments.size(); ++i)    delete _segments[i];
  for (size_t i = 0; i < _segmentsIR.size(); ++i)  delete _segmentsIR[i];
  ...
}
```

(Leave the rest of the function as-is. Do not reformat the library files.)

**Acceptance:** selecting a 10-minute WAV as a custom IR loads a truncated
20 s IR with bounded memory (< ~200 MB transient), no exception escapes, and
audio continues.

---

## Task 6 — Marshal PATH A's swap onto the message thread

**File:** `PluginProcessor.cpp`, `loadFromValueTree()` PATH A branch.

`setStateInformation` may arrive on a non-message thread; PATH A's `doSwap`
mutates `ownedModule`, which the editor's 30 Hz timer reads on the message
thread. Replace:

```cpp
if (spec.sampleRate == 0)
{
    doSwap(incoming);
}
```

with:

```cpp
if (spec.sampleRate == 0)
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        doSwap (incoming);
    }
    else
    {
        // Marshal to the message thread; slots are empty pre-prepareToPlay so
        // a tiny deferral is harmless. doSwap still takes the callback lock.
        auto shared = std::make_shared<std::vector<PendingSlot>> (std::move (incoming));
        juce::WeakReference<ADSREchoAudioProcessor> weakThis (this);
        const auto myGeneration = loadGeneration->load (std::memory_order_acquire);
        auto genPtr = loadGeneration;
        juce::MessageManager::callAsync ([weakThis, shared, genPtr, myGeneration]
        {
            if (auto* self = weakThis.get())
                if (myGeneration == genPtr->load (std::memory_order_acquire))
                    self->doSwapImpl (*shared);
        });
    }
}
```

To make this compile, hoist the `doSwap` lambda into a private member function
`void doSwapImpl (std::vector<PendingSlot>&)` (move `PendingSlot` into the
class or a detail header) and have both PATH A and the lambda call it.

**Acceptance:** no behavioural change when setState arrives on the message
thread; when forced onto a worker thread (unit-test or temporary
`std::thread` harness), modules still appear and the editor doesn't race.

---

## Task 7 — Report a real tail length

**File:** `PluginProcessor.cpp`

```cpp
double ADSREchoAudioProcessor::getTailLengthSeconds() const
{
    // Convolution + delay feedback tails. A precise per-IR value would need
    // plumbing; a generous constant stops hosts truncating bounces.
    return 25.0;   // >= kMaxIRSeconds + max preDelay (2 s) + headroom
}
```

(If you later plumb the longest loaded IR length through, prefer that.)

---

## Task 8 — Pin down `FFTCONVOLVER_USE_SSE` at project level

**Files:** build config only (`CMakeLists.txt` and/or the `.jucer`).

`fftconvolver/Utilities.h` auto-detects SSE per translation unit and switches
`Buffer<T>` between `_mm_malloc/_mm_free` and `new[]/delete[]` in inline code.
If TU flags ever diverge, the linker merges mismatched allocators → heap
corruption. Make it explicit and global:

- CMake: `target_compile_definitions(<plugin_target> PRIVATE FFTCONVOLVER_DONT_USE_SSE=1)`
  (or `FFTCONVOLVER_USE_SSE=1` if every TU is guaranteed SSE flags — on x64
  the scalar path is fine; correctness over micro-speed here).
- Projucer: add the same define in Preprocessor Definitions for all configs.

Also rename ONE of the duplicate headers if convenient (`Utilities.h` at
project root → `ProjectUtilities.h`, update includes) — optional, but removes
the include-resolution ambiguity between build systems permanently.

---

## Task 9 — Small follow-ups (do last, each is a few lines)

1. `prepareToPlay` pre-load loop: skip modules whose IR load is suppressed:
   add `bool isIRLoadSuppressed() const` to `Convolution`/`ConvolutionModule`
   returning `irLoadSuppressed.load()`, and guard the `forceReloadIR` call
   with `&& ! cm->isIRLoadSuppressed()`.
2. `TwoStageFFTConvolver::reset()` has a duplicated `_tailInputFill = 0;`
   line — delete one (cosmetic).
3. `ConvolutionModule::setID (juce::String& newID)` → take `const
   juce::String&` (requires matching change in `EffectModule` base and all
   overrides — only do this if the base class is in-repo and the change is
   mechanical).
4. In `ADSREchoAudioProcessor::processBlock`, the explicit
   `ScopedLock sl (getCallbackLock())` is redundant (the wrapper already
   holds it) — leave it or remove it; if removed, note that `executeSlotMove`
   relies on the lock being held, which the wrapper guarantees.

---

## Verification checklist (run after all tasks)

1. Build both Debug and Release; zero new warnings in changed files.
2. Save a project with 2 chains × (Delay, Reverb, Convolution w/ bank IR,
   Convolution w/ custom IR), close, reopen → all modules restore, correct
   IRs, no audio-thread loads (breakpoint in `loadIRAtIndex` must only hit on
   loader/message/prepareToPlay threads).
3. Open/close the project 20× in a loop → process memory returns to baseline
   (± a few MB), no thread-count growth (watch in Process Explorer /
   Activity Monitor).
4. During playback: change IR dropdown rapidly 10×, click Browse and load a
   large IR, drag-reorder slots with the file dialog open → no crash, no
   dropout longer than a block.
5. Delete the plugin instance while a preset is loading (add a 2 s sleep in
   the loader temporarily to widen the window) → clean teardown.
6. Load a legacy state XML lacking the `irIndex` slot property → correct IR
   restored (Task 2).
7. If ASan/TSan builds are available, run scenarios 2–5 under them.
