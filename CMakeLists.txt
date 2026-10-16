cmake_minimum_required(VERSION 3.22)

project(AmpBass VERSION 1.0.0)

# ============================================================
# C++ Standard
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# JUCE - FetchContent
# ============================================================

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

# ============================================================
# Plugin Definition
# ============================================================

juce_add_plugin(AmpBass
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE AMPB
    PLUGIN_CODE AmpB
    FORMATS Standalone VST3 AU
    PRODUCT_NAME "AmpBass"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    NEEDS_JUCE_CORE TRUE
    NEEDS_JUCE_AUDIO_PROCESSORS TRUE
    NEEDS_JUCE_GUI_BASICS TRUE
    NEEDS_WEB_BROWSER TRUE

    COPY_PLUGIN_AFTER_BUILD TRUE

    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_generate_juce_header(AmpBass)

# ============================================================
# Source Files
# ============================================================

target_sources(AmpBass PRIVATE
    # Plugin core
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginProcessor.h
    Source/Plugin/PluginEditor.cpp
    Source/Plugin/PluginEditor.h
    Source/Plugin/NativeBridge.cpp
    Source/Plugin/NativeBridge.h

    # Parameters
    Source/Parameters/ParameterIDs.h
    Source/Parameters/ParameterLayout.h

    # DSP Primitives (header-only)
    Source/DSP/Primitives/DelayLine.h
    Source/DSP/Primitives/OnePoleFilter.h
    Source/DSP/Primitives/Biquad.h
    Source/DSP/Primitives/SmoothedParam.h
    Source/DSP/Primitives/LFO.h
    Source/DSP/Primitives/DryWetMixer.h

    # DSP stages (header-only)
    Source/DSP/InputGain.h
    Source/DSP/PreFilters.h
    Source/DSP/TubePreamp.h
    Source/DSP/ToneStack.h
    Source/DSP/GraphicEQ.h
    Source/DSP/MasterVolume.h
    Source/DSP/PowerSection.h
)

target_include_directories(AmpBass PRIVATE
    Source/Plugin
    Source/DSP
    Source/Parameters
)

# ============================================================
# Svelte Build Logic
# ============================================================

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist/index.html")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist/index.html"
         "<html><body>Loading...</body></html>")
endif()

add_custom_target(BuildSvelteUI ALL
    COMMAND npm install
    COMMAND npm run build
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/WebUI"
    COMMENT "Building Svelte frontend..."
    VERBATIM
)

# ============================================================
# Embed Web Resources Into Binary
# ============================================================

juce_add_binary_data(AmpBass_Resources
    SOURCES
        WebUI/dist/index.html
)

add_dependencies(AmpBass_Resources BuildSvelteUI)

# ============================================================
# Compile Definitions & Linking
# ============================================================

target_compile_definitions(AmpBass PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

if(APPLE)
    target_compile_options(AmpBass PRIVATE
        -Wno-deprecated-declarations
    )
endif()

target_link_libraries(AmpBass PRIVATE
    AmpBass_Resources
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ============================================================
# Copy Resources to Bundle (Post-Build)
# ============================================================

macro(copy_resources_to_target target_name)
    if(TARGET ${target_name})
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist"
                "$<TARGET_FILE_DIR:${target_name}>/../Resources"
        )
    endif()
endmacro()

copy_resources_to_target(AmpBass)
copy_resources_to_target(AmpBass_Standalone)

# ============================================================
# Diagnostics (DSP tests + offline VST3 host)
# ============================================================

option(AMPBASS_BUILD_TESTS "Build DSP diagnostics and offline host" OFF)
if(AMPBASS_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
