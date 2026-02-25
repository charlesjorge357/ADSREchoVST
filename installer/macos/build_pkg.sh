#!/bin/bash
set -euo pipefail

VERSION="${1:?Usage: build_pkg.sh <version>}"
BUILD_DIR="../../build/ADSREcho_artefacts/Release"
STAGING="staging"
OUTPUT_DIR="../../release"
PKG_ID_BASE="com.adsr-echo"

rm -rf "$STAGING" "$OUTPUT_DIR"
mkdir -p "$STAGING/vst3" "$STAGING/au" "$OUTPUT_DIR"

# Stage VST3
if [ -d "$BUILD_DIR/VST3/ADSREcho.vst3" ]; then
    cp -R "$BUILD_DIR/VST3/ADSREcho.vst3" "$STAGING/vst3/"
fi

# Stage AU
if [ -d "$BUILD_DIR/AU/ADSREcho.component" ]; then
    cp -R "$BUILD_DIR/AU/ADSREcho.component" "$STAGING/au/"
fi

# Build component packages
PKGS=()

if [ -d "$STAGING/vst3/ADSREcho.vst3" ]; then
    pkgbuild \
        --root "$STAGING/vst3" \
        --identifier "${PKG_ID_BASE}.vst3" \
        --version "$VERSION" \
        --install-location "/Library/Audio/Plug-Ins/VST3" \
        "$STAGING/adsr-echo-vst3.pkg"
    PKGS+=("$STAGING/adsr-echo-vst3.pkg")
fi

if [ -d "$STAGING/au/ADSREcho.component" ]; then
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
