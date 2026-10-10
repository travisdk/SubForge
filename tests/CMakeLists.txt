# JUCE-free unit tests for the DSP, sequencer, presets and patterns. Run with ctest.
foreach (name presets patterns sequencer filter_curve oscillator)
    add_executable (test_${name} test_${name}.cpp)
    target_include_directories (test_${name} PRIVATE ${PROJECT_SOURCE_DIR}/Source)
    target_compile_features (test_${name} PRIVATE cxx_std_17)
    add_test (NAME ${name} COMMAND test_${name})
endforeach()
