cmake_minimum_required(VERSION 3.15)

project(FreeCaster VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set architecture for macOS to match the host system
execute_process(
    COMMAND uname -m
    OUTPUT_VARIABLE HOST_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(CMAKE_OSX_ARCHITECTURES "${HOST_ARCH}" CACHE STRING "macOS architecture" FORCE)
message(STATUS "Building for macOS architecture: ${CMAKE_OSX_ARCHITECTURES}")

# Find OpenSSL for AirPlay authentication
if(EXISTS "/opt/homebrew/opt/openssl@3")
    set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl@3")
elseif(EXISTS "/usr/local/opt/openssl@3")
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@3")
elseif(EXISTS "/opt/homebrew/opt/openssl")
    set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl")
elseif(EXISTS "/usr/local/opt/openssl")
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()

find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(FreeCaster
    COMPANY_NAME "FreeCaster"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Frec
    PLUGIN_CODE Fcst
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "FreeCaster"
    VST3_CATEGORIES Fx Tools
    AU_MAIN_TYPE kAudioUnitType_Effect
)

target_sources(FreeCaster
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/AirPlay/AirPlayManager.cpp
        Source/AirPlay/AirPlayMac.mm
        Source/Discovery/DeviceDiscoveryMac.mm
        Source/Discovery/AirPlayDevice.cpp
        Source/Audio/AudioEncoder.cpp
        Source/Audio/ALACEncoderWrapper.cpp
        Source/Audio/StreamBuffer.cpp
        Source/Audio/ALAC/ALACEncoder.cpp
        Source/Audio/ALAC/ALACBitUtilities.c
        Source/Audio/ALAC/ag_enc.c
        Source/Audio/ALAC/ag_dec.c
        Source/Audio/ALAC/dp_enc.c
        Source/Audio/ALAC/dp_dec.c
        Source/Audio/ALAC/matrix_enc.c
        Source/Audio/ALAC/matrix_dec.c
        Source/Audio/ALAC/EndianPortable.c
)

# macOS-specific frameworks
target_link_libraries(FreeCaster PRIVATE
    "-framework AVFoundation"
    "-framework CoreAudio"
    "-framework Network"
    "-framework Foundation"
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_compile_definitions(FreeCaster
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(FreeCaster PRIVATE Source)

target_link_libraries(FreeCaster
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================================================
# FreeCaster Unit Tests
# ============================================================================

add_executable(FreeCasterTests
    Tests/TestMain.cpp
    Tests/StreamBufferTests.cpp
    Tests/AudioEncoderTests.cpp
    Tests/AirPlayDeviceTests.cpp
    # Reuse source files without GUI
    Source/AirPlay/AirPlayManager.cpp
    Source/AirPlay/AirPlayMac.mm
    Source/Discovery/AirPlayDevice.cpp
    Source/Discovery/DeviceDiscoveryMac.mm
    Source/Audio/StreamBuffer.cpp
    Source/Audio/AudioEncoder.cpp
    Source/Audio/ALACEncoderWrapper.cpp
    Source/Audio/ALAC/ALACEncoder.cpp
    Source/Audio/ALAC/ALACBitUtilities.c
    Source/Audio/ALAC/ag_enc.c
    Source/Audio/ALAC/ag_dec.c
    Source/Audio/ALAC/dp_enc.c
    Source/Audio/ALAC/dp_dec.c
    Source/Audio/ALAC/matrix_enc.c
    Source/Audio/ALAC/matrix_dec.c
    Source/Audio/ALAC/EndianPortable.c
)

target_link_libraries(FreeCasterTests PRIVATE
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    "-framework AVFoundation"
    "-framework CoreAudio"
    "-framework Foundation"
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_include_directories(FreeCasterTests PRIVATE Source)

target_compile_definitions(FreeCasterTests
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)
