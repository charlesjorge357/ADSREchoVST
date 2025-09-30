# CI/CD Pipeline Documentation

## Overview

This repository uses GitHub Actions for continuous integration and deployment. The pipeline automates building, testing, and releasing the ADSR-Echo multi-effect audio plugin across multiple platforms.

## Workflows

### 1. CI Workflow (`.github/workflows/ci.yml`)

**Triggers:**
- Push to `main`, `develop`, or `feature/**` branches
- Pull requests to `main` or `develop`

**Jobs:**

#### Lint
- Runs `clang-format` to check code style
- Ensures consistent C++ formatting across the codebase

#### Build Matrix
Builds the plugin for multiple platforms:
- **Windows** (MSVC, VST3)
- **macOS Intel** (x86_64, VST3/AU)
- **macOS ARM** (arm64, VST3/AU)
- **Linux** (GCC, VST3)

#### Test
- Runs Catch2 unit tests
- Executes audio DSP validation tests
- Reports test coverage

#### PluginVal
- Validates VST3/AU plugin standards compliance
- Runs on all platforms to ensure compatibility

**Artifacts:** Debug builds retained for 30 days

---

### 2. Release Workflow (`.github/workflows/release.yml`)

**Triggers:**
- Git tags matching `v*.*.*` (e.g., `v1.0.0`)

**Jobs:**

#### Build Release
- Builds optimized Release binaries for all platforms
- Creates universal macOS binaries (Intel + ARM)
- Packages plugins as platform-specific archives:
  - Windows: `.zip`
  - macOS: `.zip`
  - Linux: `.tar.gz`

#### Create Release
- Generates GitHub Release with changelog
- Uploads all platform packages
- Tags release with version number

**Usage:**
```bash
git tag v1.0.0
git push origin v1.0.0
```

---

### 3. ML Training Workflow (`.github/workflows/ml-training.yml`)

**Triggers:**
- Manual dispatch (workflow_dispatch)
- Changes to `ml/**` directory
- Push to `main` or `develop`

**Jobs:**

#### Train Model
- Sets up Python environment with PyTorch
- Trains reverb IR generation model
- Exports model to ONNX format
- Validates ONNX model with ONNX Runtime

#### Test ONNX Integration
- Tests ONNX model loading on Windows/macOS/Linux
- Validates C++ ONNX Runtime integration
- Benchmarks inference performance

#### Publish Models
- Uploads ONNX models as artifacts
- Creates ML model releases for plugin integration

**Manual Trigger:**
```bash
gh workflow run ml-training.yml -f model_name=reverb_ir_generator -f epochs=100
```

---

## Build System

### CMake Configuration

The project uses CMake with JUCE framework:

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Build with tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --output-on-failure
```

### Plugin Formats

- **VST3**: All platforms
- **AU**: macOS only
- **AAX**: Requires AAX SDK (not included)

---

## Testing

### Unit Tests (Catch2)

Located in `Tests/` directory:
- `PluginBasicTests.cpp` - Plugin instantiation and state management
- `DSPTests.cpp` - Audio processing algorithms

Run tests locally:
```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --verbose
```

### Audio Test Harness

Tests include:
- Signal processing correctness
- Null tests (bypass mode)
- Frequency response validation
- THD measurements
- Performance benchmarks

---

## Code Quality

### Formatting

Code style is enforced using `.clang-format`:

```bash
# Check formatting
find Source -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror

# Auto-format
find Source -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### Static Analysis

Future enhancements:
- clang-tidy for static analysis
- cppcheck for additional checks
- Coverage reporting with lcov/gcov

---

## Dependencies

### Build Dependencies

**All Platforms:**
- CMake 3.15+
- C++17 compiler

**Linux:**
```bash
sudo apt-get install libasound2-dev libjack-jackd2-dev libfreetype6-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev
```

**macOS:**
- Xcode Command Line Tools

**Windows:**
- Visual Studio 2019+ or Build Tools

### Python Dependencies (ML)

```bash
pip install torch torchvision torchaudio onnx onnxruntime numpy scipy librosa
```

---

## Caching

The CI uses GitHub Actions cache for:
- CMake build artifacts
- JUCE modules
- Python pip packages

This significantly speeds up build times on subsequent runs.

---

## Security

### Secrets (Future)

For code signing and deployment:
- `WINDOWS_CERT_PASSWORD` - Windows code signing certificate
- `MACOS_CERT_P12` - macOS Developer ID certificate
- `MACOS_CERT_PASSWORD` - Certificate password
- `MACOS_NOTARIZATION_USERNAME` - Apple ID for notarization
- `MACOS_NOTARIZATION_PASSWORD` - App-specific password

Add via: Repository Settings → Secrets → Actions

---

## Performance Gates

The CI enforces:
- ✅ All tests must pass
- ✅ No formatting violations
- ✅ Plugin validator must pass
- ✅ Build must complete in <20 minutes

Future gates:
- CPU usage benchmarks
- Memory leak detection
- Latency measurements <10ms

---

## Troubleshooting

### Build Failures

1. **Submodule issues**: Ensure JUCE submodule is initialized
   ```bash
   git submodule update --init --recursive
   ```

2. **CMake version**: Requires CMake 3.15+
   ```bash
   cmake --version
   ```

3. **Missing dependencies**: Install platform-specific libraries

### Test Failures

- Check test logs in GitHub Actions artifacts
- Run tests locally for debugging
- Verify audio processing algorithms

---

## Contributing

When contributing:
1. Format code with `clang-format` before committing
2. Add tests for new features
3. Ensure CI passes before requesting review
4. Update this documentation for significant changes

---

## Future Enhancements

- [ ] Automated installer creation (NSIS, pkgbuild)
- [ ] Code signing integration
- [ ] macOS notarization
- [ ] Performance profiling reports
- [ ] Test coverage badges
- [ ] Automated changelog generation
- [ ] Docker containers for reproducible builds
