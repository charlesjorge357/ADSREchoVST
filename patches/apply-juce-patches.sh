#!/usr/bin/env sh
# Apply every JUCE patch in this directory to the JUCE submodule.
#
# For Projucer builds, which do NOT run the CMake auto-apply step. Run this
# once after a fresh clone (or after updating the JUCE submodule) before you
# build in Projucer / Visual Studio / Xcode.
#
# Idempotent: a patch already present is detected and skipped, so it is safe
# to re-run. Works anywhere git is installed (macOS, Linux, Git Bash on
# Windows).
set -e
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
juce_dir="$script_dir/../JUCE"

for patch in "$script_dir"/*.patch; do
    [ -e "$patch" ] || continue
    name=$(basename "$patch")
    if git -C "$juce_dir" apply --reverse --check "$patch" >/dev/null 2>&1; then
        echo "already applied: $name"
    elif git -C "$juce_dir" apply --check "$patch" >/dev/null 2>&1; then
        git -C "$juce_dir" apply "$patch"
        echo "applied:         $name"
    else
        echo "WARNING: does not apply cleanly (JUCE version changed?): $name" >&2
        echo "         Regenerate it — see patches/README.md." >&2
    fi
done
