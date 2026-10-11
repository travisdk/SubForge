cmake_minimum_required(VERSION 3.22)
project(SubForge VERSION 0.1.0)

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
    FORMATS Standalone VST3
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

if (SUBFORGE_INSTALL_AFTER_BUILD)
    add_custom_command(TARGET SubForge_CLAP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CLAP_INSTALL_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SubForge_CLAP> ${CLAP_INSTALL_DIR}/
        COMMENT "Installing SubForge.clap into ${CLAP_INSTALL_DIR}")
endif()

# Release archive with the CLAP and VST3 plug-ins, the third-party notices and
# install notes: dist/SubForge-<version>-linux-x86_64.tar.gz on Linux,
# dist/SubForge-<version>-windows-x64.zip on Windows.
if (WIN32)
    set(SUBFORGE_PACKAGE_TARGET package-windows)
    set(SUBFORGE_PACKAGE_NAME "SubForge-${PROJECT_VERSION}-windows-x64")
    set(SUBFORGE_PACKAGE_FILE "${SUBFORGE_PACKAGE_NAME}.zip")
    set(SUBFORGE_TAR_FLAGS cf)
    set(SUBFORGE_PACKAGE_FORMAT --format=zip)
    set(SUBFORGE_INSTALL_NOTES "${CMAKE_SOURCE_DIR}/packaging/INSTALL-windows.txt")
else()
    set(SUBFORGE_PACKAGE_TARGET package-linux)
    set(SUBFORGE_PACKAGE_NAME "SubForge-${PROJECT_VERSION}-linux-x86_64")
    set(SUBFORGE_PACKAGE_FILE "${SUBFORGE_PACKAGE_NAME}.tar.gz")
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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt ${SUBFORGE_PACKAGE_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${SUBFORGE_INSTALL_NOTES} ${SUBFORGE_PACKAGE_DIR}/INSTALL.txt
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/dist
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/package
            ${CMAKE_COMMAND} -E tar ${SUBFORGE_TAR_FLAGS} ${CMAKE_SOURCE_DIR}/dist/${SUBFORGE_PACKAGE_FILE} ${SUBFORGE_PACKAGE_FORMAT} ${SUBFORGE_PACKAGE_NAME}
    DEPENDS SubForge_CLAP SubForge_VST3
    COMMENT "Packaging dist/${SUBFORGE_PACKAGE_FILE}"
    VERBATIM)
