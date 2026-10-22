# Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
# SPDX-License-Identifier: AGPL-3.0-only

# Unit tests, run with ctest. All but test_loading are JUCE-free and test the DSP,
# sequencer, presets and patterns on their own.
foreach (name presets patterns sequencer filter_curve oscillator voice levels)
    add_executable (test_${name} test_${name}.cpp)
    target_include_directories (test_${name} PRIVATE ${PROJECT_SOURCE_DIR}/Source)
    target_compile_features (test_${name} PRIVATE cxx_std_17)
    add_test (NAME ${name} COMMAND test_${name})
endforeach()

# Loading damaged or hostile projects and files through the real processor; links the
# plug-in code and JUCE (the SubForge target is defined after this folder, which CMake allows).
add_executable (test_loading test_loading.cpp)
target_include_directories (test_loading PRIVATE ${PROJECT_SOURCE_DIR}/Source $<TARGET_PROPERTY:SubForge,INCLUDE_DIRECTORIES>)
target_compile_definitions (test_loading PRIVATE $<TARGET_PROPERTY:SubForge,COMPILE_DEFINITIONS>)
target_link_libraries (test_loading PRIVATE SubForge)
add_test (NAME loading COMMAND test_loading)
