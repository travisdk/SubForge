# Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.22)
project(SubForge VERSION 1.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MSVC: link the C++ runtime statically (/MT). A plug-in runs inside the host's process, so a
# dynamic MSVCP140.dll would have to be installed on the user's PC and could be an older version
# already loaded by the host. Set before any target is created, so JUCE and the CLAP wrapper match.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# SDKs default to ~/SDKs (Linux, macOS) or %USERPROFILE%\SDKs (Windows); override with -DJUCE_DIR=... etc.
if (WIN32)
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" SUBFORGE_HOME)
else()
    set(SUBFORGE_HOME "$ENV{HOME}")
endif()
set(JUCE_DIR "${SUBFORGE_HOME}/SDKs/JUCE-8.0.15" CACHE PATH "JUCE source tree")
set(CLAP_JUCE_EXTENSIONS_DIR "${SUBFORGE_HOME}/SDKs/clap-juce-extensions-main" CACHE PATH "clap-juce-extensions source tree")

# Per-user plug-in folders from the CLAP and VST3 specifications (no administrator rights needed)
if (WIN32)
    file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}" SUBFORGE_LOCALAPPDATA)
    set(SUBFORGE_DEFAULT_CLAP_DIR "${SUBFORGE_LOCALAPPDATA}/Programs/Common/CLAP")
    set(SUBFORGE_DEFAULT_VST3_DIR "${SUBFORGE_LOCALAPPDATA}/Programs/Common/VST3")
elseif (APPLE)
    set(SUBFORGE_DEFAULT_CLAP_DIR "${SUBFORGE_HOME}/Library/Audio/Plug-Ins/CLAP")
    set(SUBFORGE_DEFAULT_VST3_DIR "${SUBFORGE_HOME}/Library/Audio/Plug-Ins/VST3")
else()
    set(SUBFORGE_DEFAULT_CLAP_DIR "${SUBFORGE_HOME}/.clap")
    set(SUBFORGE_DEFAULT_VST3_DIR "${SUBFORGE_HOME}/.vst3")
endif()
option(SUBFORGE_INSTALL_AFTER_BUILD "Copy the CLAP and VST3 into the user's plug-in folders after each build" ON)
set(CLAP_INSTALL_DIR "${SUBFORGE_DEFAULT_CLAP_DIR}" CACHE PATH "Where the built .clap is copied")
set(VST3_INSTALL_DIR "${SUBFORGE_DEFAULT_VST3_DIR}" CACHE PATH "Where the built .vst3 is copied")

add_subdirectory(${JUCE_DIR} JUCE)

option(SUBFORGE_BUILD_TESTS "Build the JUCE-free unit tests (run with ctest)" ON)
if (SUBFORGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
add_subdirectory(${CLAP_JUCE_EXTENSIONS_DIR} clap-juce-extensions EXCLUDE_FROM_ALL)

juce_add_plugin(SubForge
    COMPANY_NAME "Henrik Juhl"
    BUNDLE_ID dk.hjuhl.subforge
    PLUGIN_MANUFACTURER_CODE Hjuh
    PLUGIN_CODE SbFg
    PRODUCT_NAME "SubForge"
    FORMATS VST3
    VST3_CATEGORIES Instrument Synth Mono
    COPY_PLUGIN_AFTER_BUILD ${SUBFORGE_INSTALL_AFTER_BUILD}   # VST3 (CLAP is copied by the command further down)
    VST3_COPY_DIR "${VST3_INSTALL_DIR}"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE)

target_sources(SubForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SubForgeLookAndFeel.cpp
    Source/StepGrid.cpp
    Source/AboutOverlay.cpp
    Source/MainView.cpp
    Source/Widgets.cpp
    Source/SoundPresets.cpp
    Source/PatternPresets.cpp)

# SUBFORGE_VERSION_STRING in SubForgeVersion.h: "git describe --tags --dirty", refreshed at
# every build so it always names the exact commit; project VERSION when git is unavailable.
find_package(Git QUIET)
set(SUBFORGE_VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/SubForgeVersion.h")
set(SUBFORGE_VERSION_COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DOUTPUT=${SUBFORGE_VERSION_HEADER}
    -DFALLBACK=${PROJECT_VERSION} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GitVersion.cmake)
execute_process(COMMAND ${SUBFORGE_VERSION_COMMAND}) # so the header exists before the first build
file(STRINGS ${SUBFORGE_VERSION_HEADER} SUBFORGE_VERSION REGEX "SUBFORGE_VERSION_STRING")
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" SUBFORGE_VERSION "${SUBFORGE_VERSION}") # as of configuring, for the package name
add_custom_target(SubForgeVersion COMMAND ${SUBFORGE_VERSION_COMMAND} BYPRODUCTS ${SUBFORGE_VERSION_HEADER}
    COMMENT "Checking the SubForge version")
add_dependencies(SubForge SubForgeVersion)
target_include_directories(SubForge PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/generated)

juce_add_binary_data(SubForgeAssets SOURCES
    Resources/Fonts/ChakraPetch/ChakraPetch-SemiBold.ttf
    Resources/Fonts/ChakraPetch/ChakraPetch-Bold.ttf
    Resources/Fonts/JetBrainsMono/JetBrainsMono-Medium.ttf
    THIRD-PARTY-NOTICES.txt)
set_target_properties(SubForgeAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_compile_definitions(SubForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0)  # no VST2 SDK; VST3 only

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

# Renders the README screenshot (not part of a normal build):
#   cmake --build build --target SubForge_Screenshot && build/SubForge_Screenshot docs/images/screenshot.png
add_executable(SubForge_Screenshot EXCLUDE_FROM_ALL tools/screenshot.cpp)
target_include_directories(SubForge_Screenshot PRIVATE Source $<TARGET_PROPERTY:SubForge,INCLUDE_DIRECTORIES>)
target_compile_definitions(SubForge_Screenshot PRIVATE $<TARGET_PROPERTY:SubForge,COMPILE_DEFINITIONS>)
target_link_libraries(SubForge_Screenshot PRIVATE SubForge)

# Draws the square wave at several Sag settings for the README (not part of a normal build):
#   cmake --build build --target SubForge_WaveShapes && build/SubForge_WaveShapes docs/images/square-shaping.png
add_executable(SubForge_WaveShapes EXCLUDE_FROM_ALL tools/wave_shapes.cpp)
target_include_directories(SubForge_WaveShapes PRIVATE Source $<TARGET_PROPERTY:SubForge,INCLUDE_DIRECTORIES>)
target_compile_definitions(SubForge_WaveShapes PRIVATE $<TARGET_PROPERTY:SubForge,COMPILE_DEFINITIONS>)
target_link_libraries(SubForge_WaveShapes PRIVATE SubForge)

# Renders the audio demo (not part of a normal build):
#   cmake --build build --target SubForge_RenderDemo && build/SubForge_RenderDemo demo.wav timecodes.txt
add_executable(SubForge_RenderDemo EXCLUDE_FROM_ALL tools/render_demo.cpp)
target_include_directories(SubForge_RenderDemo PRIVATE Source $<TARGET_PROPERTY:SubForge,INCLUDE_DIRECTORIES>)
target_compile_definitions(SubForge_RenderDemo PRIVATE $<TARGET_PROPERTY:SubForge,COMPILE_DEFINITIONS>)
target_link_libraries(SubForge_RenderDemo PRIVATE SubForge)

if (SUBFORGE_INSTALL_AFTER_BUILD)
    add_custom_command(TARGET SubForge_CLAP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CLAP_INSTALL_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SubForge_CLAP> ${CLAP_INSTALL_DIR}/
        COMMENT "Installing SubForge.clap into ${CLAP_INSTALL_DIR}")
endif()

# Release archive with the CLAP and VST3 plug-ins, the licence, the changelog,
# the third-party notices and install notes: dist/SubForge-linux-x86_64.tar.gz on Linux,
# dist/SubForge-windows-x64.zip on Windows. The file names carry no version so the README can
# link to the latest release; the folder inside the archive does, from "git describe" when
# configuring (CI configures from scratch; reconfigure a local build before packaging).
if (WIN32)
    set(SUBFORGE_PACKAGE_TARGET package-windows)
    set(SUBFORGE_PACKAGE_NAME "SubForge-${SUBFORGE_VERSION}-windows-x64")
    set(SUBFORGE_PACKAGE_FILE "SubForge-windows-x64.zip")
    set(SUBFORGE_TAR_FLAGS cf)
    set(SUBFORGE_PACKAGE_FORMAT --format=zip)
    set(SUBFORGE_INSTALL_NOTES "${CMAKE_SOURCE_DIR}/packaging/INSTALL-windows.txt")
else()
    set(SUBFORGE_PACKAGE_TARGET package-linux)
    set(SUBFORGE_PACKAGE_NAME "SubForge-${SUBFORGE_VERSION}-linux-x86_64")
    set(SUBFORGE_PACKAGE_FILE "SubForge-linux-x86_64.tar.gz")
    set(SUBFORGE_TAR_FLAGS cfz)
    set(SUBFORGE_PACKAGE_FORMAT --format=gnutar)
    set(SUBFORGE_INSTALL_NOTES "${CMAKE_SOURCE_DIR}/packaging/INSTALL-linux.txt")
endif()
set(SUBFORGE_PACKAGE_DIR "${CMAKE_BINARY_DIR}/package/${SUBFORGE_PACKAGE_NAME}")
add_custom_target(${SUBFORGE_PACKAGE_TARGET}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${SUBFORGE_PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SUBFORGE_PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SubForge_CLAP> ${SUBFORGE_PACKAGE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:SubForge_VST3>/../.. ${SUBFORGE_PACKAGE_DIR}/SubForge.vst3
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/LICENSE ${SUBFORGE_PACKAGE_DIR}/LICENSE.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/CHANGELOG.md ${SUBFORGE_PACKAGE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt ${SUBFORGE_PACKAGE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${SUBFORGE_INSTALL_NOTES} ${SUBFORGE_PACKAGE_DIR}/INSTALL.txt
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/dist
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/package
            ${CMAKE_COMMAND} -E tar ${SUBFORGE_TAR_FLAGS} ${CMAKE_SOURCE_DIR}/dist/${SUBFORGE_PACKAGE_FILE} ${SUBFORGE_PACKAGE_FORMAT} ${SUBFORGE_PACKAGE_NAME}
    DEPENDS SubForge_CLAP SubForge_VST3
    COMMENT "Packaging dist/${SUBFORGE_PACKAGE_FILE}"
    VERBATIM)
