# Tests CMakeLists.txt

# Fetch Catch2
Include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(Catch2)

# Create test executable
add_executable(ADSREchoTests
    PluginBasicTests.cpp
    DSPTests.cpp
)

# Link with Catch2 and plugin code
target_link_libraries(ADSREchoTests
    PRIVATE
        Catch2::Catch2WithMain
        ADSREcho
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_dsp
)

# Include directories
target_include_directories(ADSREchoTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
)

# Compile definitions to match the plugin
target_compile_definitions(ADSREchoTests
    PRIVATE
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_UNIT_TESTS=1
)

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(ADSREchoTests)
