# JUCE patches

`JUCE/` is a git **submodule** pinned to a pristine upstream commit. We never
commit changes into the submodule (and we can't push to `juce-framework/JUCE`
anyway). Instead, every local fix to JUCE lives here as a `.patch` file and is
re-applied on top of the pristine tree at build time.

## How they get applied

**CMake builds (and CI):** the top-level `CMakeLists.txt` applies every
`*.patch` in this directory at configure time, before `add_subdirectory(JUCE)`.
It is idempotent — a patch already applied is skipped, so re-running CMake is
safe. CI applies patches the same way (it runs CMake). Nothing to do by hand.

**Projucer builds:** Projucer / the generated Visual Studio / Xcode projects do
**not** run CMake, so the patches are *not* applied automatically. After a fresh
clone or after updating the JUCE submodule, run the apply script once before
building:

```sh
# macOS / Linux / Git Bash on Windows
sh patches/apply-juce-patches.sh
```
```powershell
# Windows PowerShell
powershell -ExecutionPolicy Bypass -File patches\apply-juce-patches.ps1
```

Both scripts are idempotent (a patch already present is detected and skipped)
and use `git apply`, so they need nothing beyond git. You only need to re-run
them after the JUCE submodule is reset or updated — once applied, the change
stays on disk in your working tree.

> Without this, a Projucer build compiles JUCE **without** the HeapBlock
> double-free fix and the 0xc0000374 heap-corruption crash (CLAUDE.md Issue #6)
> comes back silently. When in doubt, just re-run the script — it's safe.

## Current patches

| File | Fixes |
|---|---|
| `juce-heapblock-doublefree-fix.patch` | `HeapBlock::malloc/calloc/allocate` null `data` before reallocating, so a `std::bad_alloc` under memory pressure doesn't leave a dangling pointer that gets double-freed (0xc0000374). See CLAUDE.md Issue #6. |
| `juce-xcode15-iterator-fix.patch` | Adds iterator traits to `StrideIterator` for Xcode 15+ builds. |

## After a JUCE upgrade

Patches are pinned to specific line context and may no longer apply. If CMake
warns that a patch doesn't apply cleanly:

1. Re-apply the change by hand in the JUCE working tree.
2. Regenerate the patch:
   ```sh
   cd JUCE
   git diff <path/to/changed/file> > ../patches/<name>.patch
   ```
3. Reset the submodule working tree so the change lives *only* in the patch
   file, not as an uncommitted submodule edit:
   ```sh
   cd JUCE && git checkout -- <path/to/changed/file>
   ```

## Regenerating a patch from a current working-tree edit

```sh
cd JUCE
git diff modules/juce_core/memory/juce_HeapBlock.h > ../patches/juce-heapblock-doublefree-fix.patch
git checkout -- modules/juce_core/memory/juce_HeapBlock.h   # revert submodule; patch now owns the change
```
