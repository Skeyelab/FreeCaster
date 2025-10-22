cmake_minimum_required(VERSION 3.15)

project(FreeCaster VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL for AirPlay authentication
find_package(OpenSSL REQUIRED)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

# Set platform-specific sources
if(APPLE)
    set(PLATFORM_SOURCES Source/AirPlay/AirPlayMac.mm)
elseif(WIN32)
    set(PLATFORM_SOURCES Source/AirPlay/AirPlayWindows.cpp)
else()
    set(PLATFORM_SOURCES Source/AirPlay/AirPlayLinux.cpp)
endif()

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
        Source/AirPlay/AirPlayAuth.cpp
        Source/AirPlay/RaopClient.cpp
        Source/Discovery/DeviceDiscovery.cpp
        Source/Discovery/AirPlayDevice.cpp
        Source/Audio/AudioEncoder.cpp
        Source/Audio/StreamBuffer.cpp
        ${PLATFORM_SOURCES}
)

# Add platform-specific discovery
if(APPLE)
    target_sources(FreeCaster PRIVATE Source/Discovery/DeviceDiscoveryMac.mm)
endif()

if(APPLE)
    target_link_libraries(FreeCaster PRIVATE
        "-framework AVFoundation"
        "-framework CoreAudio"
        "-framework Network"
        "-framework Foundation"
        OpenSSL::SSL
        OpenSSL::Crypto
    )
elseif(WIN32)
    target_link_libraries(FreeCaster PRIVATE 
        ws2_32 
        dnsapi 
        iphlpapi
        OpenSSL::SSL
        OpenSSL::Crypto
    )
else()
    target_link_libraries(FreeCaster PRIVATE 
        avahi-client 
        avahi-common
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

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
