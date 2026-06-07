cmake_minimum_required(VERSION 3.22)
project(CRiSPY VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# Option to build Standalone (original Win32 app) or VST3 plugin
# ==============================================================================
option(BUILD_VST3 "Build VST3 plugin using JUCE" ON)
option(BUILD_STANDALONE_APP "Build standalone Win32 app" ON)

# ==============================================================================
# STANDALONE APP (original Win32 Direct2D application)
# ==============================================================================
if(BUILD_STANDALONE_APP)
    set(STANDALONE_SOURCES
        src/main.cpp
        src/drag_drop.h
        src/drag_drop.cpp
    )

    add_executable(NvidiaStudioVoiceEnhancer WIN32 ${STANDALONE_SOURCES})

    target_link_libraries(NvidiaStudioVoiceEnhancer PRIVATE
        d2d1 dwrite shlwapi ole32 uuid Windowscodecs
    )

    if(MSVC)
        target_compile_options(NvidiaStudioVoiceEnhancer PRIVATE /utf-8 /W4)
    endif()
endif()

# ==============================================================================
# VST3 PLUGIN (JUCE-based)
# ==============================================================================
if(BUILD_VST3)
    # Add JUCE as a subdirectory
    add_subdirectory(JUCE)

    # Embed Crispy.png as binary resource
    juce_add_binary_data(CRiSPYBinaryData SOURCES
        Crispy.png
    )

    # Create the VST3 plugin
    juce_add_plugin(CRiSPYPlugin
        PRODUCT_NAME "CRiSPY"
        COMPANY_NAME "imLeGEnDco"
        PLUGIN_MANUFACTURER_CODE ImLG
        PLUGIN_CODE Crsp
        FORMATS VST3
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES "Fx" "Restoration"
    )

    # Plugin source files
    target_sources(CRiSPYPlugin PRIVATE
        src/PluginProcessor.h
        src/PluginProcessor.cpp
        src/PluginEditor.h
        src/PluginEditor.cpp
    )

    # Generate JuceHeader.h
    juce_generate_juce_header(CRiSPYPlugin)

    # Compile definitions required by JUCE
    target_compile_definitions(CRiSPYPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
    )

    # Link JUCE modules and binary data
    target_link_libraries(CRiSPYPlugin PRIVATE
        CRiSPYBinaryData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    if(MSVC)
        target_compile_options(CRiSPYPlugin PRIVATE /utf-8)
    endif()
endif()
