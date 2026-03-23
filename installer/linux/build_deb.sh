#!/bin/bash
set -euo pipefail

VERSION="${1:?Usage: build_deb.sh <version>}"
BUILD_DIR="../../build-host/ADSREcho_artefacts/VST3"
STAGING="staging"
OUTPUT_DIR="../../release"
ARCH="amd64"
PKG_NAME="adsr-echo"
INSTALL_DIR="/usr/lib/vst3"

# Verify build exists
if [ ! -d "$BUILD_DIR/ADSR-Echo.vst3" ]; then
    echo "Error: Built VST3 not found at $BUILD_DIR/ADSR-Echo.vst3"
    echo "Run: cmake --build ../../build-host first"
    exit 1
fi

rm -rf "$STAGING" "$OUTPUT_DIR"
mkdir -p "$STAGING/DEBIAN"
mkdir -p "$STAGING${INSTALL_DIR}"
mkdir -p "$OUTPUT_DIR"

# Stage VST3 bundle (includes IRs and Presets copied by post-build step)
cp -R "$BUILD_DIR/ADSR-Echo.vst3" "$STAGING${INSTALL_DIR}/"

# Write package control file
cat > "$STAGING/DEBIAN/control" <<EOF
Package: ${PKG_NAME}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: ADSR-Echo Team <adsr-echo@example.com>
Description: ADSR-Echo VST3 Plugin
 A JUCE-based VST3 audio effect plugin with reverb, delay, EQ,
 and compressor modules. Includes convolution reverb with impulse
 response library.
Section: sound
Priority: optional
EOF

# Build .deb
DEB_FILE="${OUTPUT_DIR}/${PKG_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --build "$STAGING" "$DEB_FILE"

rm -rf "$STAGING"
echo "Created: $DEB_FILE"
echo ""
echo "Install with:   sudo dpkg -i $DEB_FILE"
echo "Uninstall with: sudo dpkg -r ${PKG_NAME}"
