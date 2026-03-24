#!/bin/bash
set -euo pipefail

VERSION="${1:?Usage: build_pkg.sh <version>}"
BUILD_DIR="../../build-host/ADSREcho_artefacts/Release"
STAGING="staging"
OUTPUT_DIR="../../release"
PKG_ID_BASE="com.adsr-echo"

rm -rf "$STAGING" "$OUTPUT_DIR"
mkdir -p "$STAGING/vst3" "$STAGING/au" "$OUTPUT_DIR"

# Stage VST3
if [ -d "$BUILD_DIR/VST3/ADSR-Echo.vst3" ]; then
    cp -R "$BUILD_DIR/VST3/ADSR-Echo.vst3" "$STAGING/vst3/"
fi

# Stage AU
if [ -d "$BUILD_DIR/AU/ADSR-Echo.component" ]; then
    cp -R "$BUILD_DIR/AU/ADSR-Echo.component" "$STAGING/au/"
fi

# Ensure IRs and Presets are in each staged bundle (fallback if post-build missed them)
RES_DIR="../../Source"
for BUNDLE in "$STAGING/vst3/ADSR-Echo.vst3" "$STAGING/au/ADSR-Echo.component"; do
    if [ -d "$BUNDLE" ]; then
        mkdir -p "$BUNDLE/Contents/Resources"
        [ -d "$BUNDLE/Contents/Resources/IRs" ]     || cp -R "$RES_DIR/IRs"     "$BUNDLE/Contents/Resources/IRs"
        [ -d "$BUNDLE/Contents/Resources/Presets" ]  || cp -R "$RES_DIR/Presets" "$BUNDLE/Contents/Resources/Presets"
    fi
done

# Build component packages
PKGS=()

if [ -d "$STAGING/vst3/ADSR-Echo.vst3" ]; then
    pkgbuild \
        --root "$STAGING/vst3" \
        --identifier "${PKG_ID_BASE}.vst3" \
        --version "$VERSION" \
        --install-location "/Library/Audio/Plug-Ins/VST3" \
        "$STAGING/adsr-echo-vst3.pkg"
    PKGS+=("$STAGING/adsr-echo-vst3.pkg")
fi

if [ -d "$STAGING/au/ADSR-Echo.component" ]; then
    pkgbuild \
        --root "$STAGING/au" \
        --identifier "${PKG_ID_BASE}.au" \
        --version "$VERSION" \
        --install-location "/Library/Audio/Plug-Ins/Components" \
        "$STAGING/adsr-echo-au.pkg"
    PKGS+=("$STAGING/adsr-echo-au.pkg")
fi

# Combine into a single product installer
productbuild \
    $(printf -- "--package %s " "${PKGS[@]}") \
    "$OUTPUT_DIR/ADSREcho-macOS-Installer-${VERSION}.pkg"

rm -rf "$STAGING"
echo "Created: $OUTPUT_DIR/ADSREcho-macOS-Installer-${VERSION}.pkg"
