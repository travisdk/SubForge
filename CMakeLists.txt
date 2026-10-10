cmake_minimum_required(VERSION 3.22)
project(SubForge VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(JUCE_DIR "$ENV{HOME}/SDKs/JUCE" CACHE PATH "JUCE source tree")
set(CLAP_JUCE_EXTENSIONS_DIR "$ENV{HOME}/SDKs/clap-juce-extensions" CACHE PATH "clap-juce-extensions source tree")
set(CLAP_INSTALL_DIR "$ENV{HOME}/.clap" CACHE PATH "Where the built .clap is copied")

add_subdirectory(${JUCE_DIR} JUCE)
add_subdirectory(${CLAP_JUCE_EXTENSIONS_DIR} clap-juce-extensions EXCLUDE_FROM_ALL)

juce_add_plugin(SubForge
    COMPANY_NAME "Henrik Juhl"
    BUNDLE_ID dk.hjuhl.subforge
    PLUGIN_MANUFACTURER_CODE Hjuh
    PLUGIN_CODE SbFg
    PRODUCT_NAME "SubForge"
    FORMATS Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE)

target_sources(SubForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SubForgeLookAndFeel.cpp
    Source/StepGrid.cpp)

juce_add_binary_data(SubForgeAssets SOURCES
    Resources/Fonts/Inter-Medium.otf
    Resources/Fonts/Inter-Bold.otf)
set_target_properties(SubForgeAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_compile_definitions(SubForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(SubForge
    PRIVATE
        SubForgeAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

clap_juce_extensions_plugin(TARGET SubForge
    CLAP_ID "dk.hjuhl.subforge"
    CLAP_FEATURES instrument synthesizer mono)

add_custom_command(TARGET SubForge_CLAP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CLAP_INSTALL_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SubForge_CLAP> ${CLAP_INSTALL_DIR}/
    COMMENT "Installing SubForge.clap into ${CLAP_INSTALL_DIR}")
