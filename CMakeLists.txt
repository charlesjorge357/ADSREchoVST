cmake_minimum_required(VERSION 3.15)

project(ADSREchoVST VERSION 0.1.0)

# C++17 is required for JUCE
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS Xcode 15+ compatibility
if(APPLE)
    add_compile_definitions(_LIBCPP_ENABLE_CXX17_REMOVED_ITERATOR_BASE=1)
endif()

# Add JUCE
add_subdirectory(JUCE)

# Plugin formats to build
set(PLUGIN_FORMATS VST3)
set(PLUGIN_FORMATS VST3 Standalone)

# Add AU format on macOS
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

# AAX format (requires AAX SDK)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDKs/AAX")
    list(APPEND PLUGIN_FORMATS AAX)
    juce_set_aax_sdk_path("${CMAKE_CURRENT_SOURCE_DIR}/SDKs/AAX")
endif()

# Define the plugin
juce_add_plugin(ADSREcho
    COMPANY_NAME "ADSR-Echo Team"
    PLUGIN_MANUFACTURER_CODE Adsr
    PLUGIN_CODE Aech
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "ADSR-Echo"

    # Plugin characteristics
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # Copy plugin to standard locations for testing
    COPY_PLUGIN_AFTER_BUILD TRUE

    # VST3 specific
    VST3_CATEGORIES "Fx" "Reverb" "Delay"

    # AU specific
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

# Source files
target_sources(ADSREcho
    PRIVATE
        Source/fftconvolver/AudioFFT.cpp
        Source/fftconvolver/FFTConvolver.cpp
        Source/fftconvolver/TwoStageFFTConvolver.cpp
        Source/fftconvolver/Utilities.cpp
        Source/fftconvolver/pffft/pffft.c
        Source/fftconvolver/pffft/pffft_common.c
        Source/BasicCompressor.cpp
	Source/BasicDelay.cpp
	Source/BasicEQ.cpp
	Source/Clookandfeel.cpp
	Source/CompressorDisplayComponent.cpp
	Source/CompressorModule.cpp
	Source/BaseModuleSlotEditor.cpp
	Source/CompressorModuleSlotEditor.cpp
	Source/CompressorPanel.cpp
	Source/Convolution.cpp
	Source/ConvolutionModule.cpp
	Source/ConvolutionPanel.cpp
	Source/CustomDelays.cpp
	Source/DatorroHall.cpp
	Source/DelayModule.cpp
	Source/DelayPanel.cpp
	Source/EQDisplayComponent.cpp
	Source/EQModule.cpp
	Source/EQModuleSlotEditor.cpp
	Source/EQPanel.cpp
	Source/HybridPlate.cpp
	Source/LFO.cpp
	Source/MasterPanel.cpp

	Source/ModuleSlotEditor.cpp
	Source/NonUniformConvolver.cpp
	Source/PluginEditor.cpp
	Source/PluginProcessor.cpp
	Source/PresetManager.cpp
	Source/PsychoDamping.cpp
	Source/ReverbModule.cpp
	Source/ReverbPanel.cpp)

# Binary resources (fonts, images)
juce_add_binary_data(ADSREchoBinaryData
    SOURCES
        Resources/Designer.otf
        Resources/FtAnimaRegular-z453.otf
        Resources/FtAnimaMedium-j8yG.otf
        Resources/HussarBoldObliqueOne-oPKq.otf
        Resources/GuavineDemoRegular-1jGgL.otf
        Resources/JUCEBack.png
        Resources/testing.png
)

# Compile definitions
target_compile_definitions(ADSREcho
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

# Pin FFTCONVOLVER_USE_SSE globally. Buffer<T> switches between _mm_malloc and
# new[] depending on this flag; mixing defined/undefined across TUs is an ODR
# hazard. Define it unconditionally so every translation unit agrees.
target_compile_definitions(ADSREcho PRIVATE FFTCONVOLVER_USE_SSE=1)

# FFT backend for AudioFFT: Apple Accelerate on macOS (vDSP, links the
# framework), pffft everywhere else (vendored in Source/fftconvolver/pffft,
# BSD-like license). Without either define AudioFFT falls back to scalar Ooura.
if(APPLE)
    target_compile_definitions(ADSREcho PRIVATE AUDIOFFT_APPLE_ACCELERATE=1)
    target_link_libraries(ADSREcho PRIVATE "-framework Accelerate")
else()
    target_compile_definitions(ADSREcho PRIVATE AUDIOFFT_PFFFT=1)
endif()

# Link JUCE modules
target_link_libraries(ADSREcho
    PRIVATE
        ADSREchoBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Include directories
target_include_directories(ADSREcho
    PRIVATE
        Source
        Source/fftconvolver
)

# macOS specific compiler options
if(APPLE)
    target_compile_options(ADSREcho PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
    )
endif()

# Optional: Enable testing
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Post-build: copy IRs and Presets into the built plugin bundle(s)
# This ensures both folders are present in the bundle for installed builds.
# The dev fallback in IRBank/PresetManager (walking up to Source/IRs and Source/Presets)
# still works during development without a full build.
set(_RES "${CMAKE_CURRENT_SOURCE_DIR}/Source")

foreach(_fmt VST3 Standalone)
    if(TARGET ADSREcho_${_fmt})
        if(APPLE)
            add_custom_command(TARGET ADSREcho_${_fmt} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_RES}/IRs" "$<TARGET_BUNDLE_DIR:ADSREcho_${_fmt}>/Contents/Resources/IRs"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_RES}/Presets" "$<TARGET_BUNDLE_DIR:ADSREcho_${_fmt}>/Contents/Resources/Presets"
                COMMENT "Copying IRs and Presets into ${_fmt} bundle"
            )
        else()
            add_custom_command(TARGET ADSREcho_${_fmt} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_RES}/IRs" "$<TARGET_FILE_DIR:ADSREcho_${_fmt}>/IRs"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_RES}/Presets" "$<TARGET_FILE_DIR:ADSREcho_${_fmt}>/Presets"
                COMMENT "Copying IRs and Presets next to ${_fmt} binary"
            )
        endif()
    endif()
endforeach()

if(APPLE AND TARGET ADSREcho_AU)
    add_custom_command(TARGET ADSREcho_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_RES}/IRs" "$<TARGET_BUNDLE_DIR:ADSREcho_AU>/Contents/Resources/IRs"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_RES}/Presets" "$<TARGET_BUNDLE_DIR:ADSREcho_AU>/Contents/Resources/Presets"
        COMMENT "Copying IRs and Presets into AU bundle"
    )
endif()

# Also copy IRs and Presets to the system install location used by COPY_PLUGIN_AFTER_BUILD,
# so the auto-copied plugin is complete (JUCE only copies the binary itself).
set(_PRODUCT_NAME "ADSR-Echo")
if(WIN32)
    set(_SYS_VST3 "$ENV{CommonProgramFiles}/VST3/${_PRODUCT_NAME}.vst3/Contents/x86_64-win")
elseif(APPLE)
    set(_SYS_VST3 "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/${_PRODUCT_NAME}.vst3/Contents/Resources")
    set(_SYS_AU   "$ENV{HOME}/Library/Audio/Plug-Ins/Components/${_PRODUCT_NAME}.component/Contents/Resources")
else()
    set(_SYS_VST3 "$ENV{HOME}/.vst3/${_PRODUCT_NAME}.vst3/Contents/x86_64-linux")
endif()

if(TARGET ADSREcho_VST3)
    add_custom_command(TARGET ADSREcho_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_RES}/IRs"     "${_SYS_VST3}/IRs"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_RES}/Presets" "${_SYS_VST3}/Presets"
        COMMENT "Copying IRs and Presets to system VST3 location"
    )
endif()

if(APPLE AND TARGET ADSREcho_AU)
    add_custom_command(TARGET ADSREcho_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_RES}/IRs"     "${_SYS_AU}/IRs"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_RES}/Presets" "${_SYS_AU}/Presets"
        COMMENT "Copying IRs and Presets to system AU location"
    )
endif()

# Installation
install(TARGETS ADSREcho
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
