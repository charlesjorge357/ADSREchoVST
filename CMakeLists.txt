cmake_minimum_required(VERSION 3.15)

project(ADSREchoVST VERSION 0.1.0)

# C++17 is required for JUCE
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(JUCE)

# Plugin formats to build
set(PLUGIN_FORMATS VST3)

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
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
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

# Link JUCE modules
target_link_libraries(ADSREcho
    PRIVATE
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
)

# Workaround for macOS Xcode 15+ C++20 iterator compatibility issue with JUCE
if(APPLE)
    target_compile_definitions(ADSREcho PRIVATE
        _LIBCPP_ENABLE_CXX17_REMOVED_ITERATOR_BASE=1
    )
    target_compile_options(ADSREcho PRIVATE
        -Wno-deprecated-declarations
    )
endif()

# Optional: Enable testing
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Installation
install(TARGETS ADSREcho
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
