#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Patches.h"

class SubForgeProcessor : public juce::AudioProcessor,
                          public juce::ChangeBroadcaster
{
public:
    SubForgeProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Host programs are the factory presets only.
    int getNumPrograms() override { return subforge::kNumPresets; }
    int getCurrentProgram() override { return isUserEntry (currentEntry) ? 0 : currentEntry; }
    void setCurrentProgram (int index) override { loadEntry (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Preset browser: factory presets first, then the user's preset files.
    // Everything here runs on the message thread and sends a change message.
    int getNumEntries() const { return subforge::kNumPresets + userPresets.size(); }
    juce::String getEntryName (int index) const;
    bool isUserEntry (int index) const { return index >= subforge::kNumPresets && index < getNumEntries(); }
    int getCurrentEntry() const { return currentEntry; }
    void loadEntry (int index);
    void randomizePatch();
    void mutatePatch();
    juce::String getPatchName() const { return patchName; }

    static constexpr const char* kPresetExtension = ".sfpreset";
    void rescanUserPresets();
    bool userPresetExists (const juce::String& name) const { return presetFileFor (name).existsAsFile(); }
    bool saveUserPreset (const juce::String& name);
    void deleteUserPreset (int index);
    juce::File getPresetFolder() const;
    void setPresetFolder (const juce::File& folder); // a default-constructed File restores the default
    static juce::File defaultPresetFolder();

    // Global preferences shared by all instances (accent colour, preset folder).
    juce::PropertiesFile& getSettings() { return settings; }

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::File presetFileFor (const juce::String& name) const;
    bool loadUserPresetFile (const juce::File& file);
    void setParameterValue (juce::RangedAudioParameter& param, float value);
    subforge::PatchValues currentPatch() const;
    void applyPatch (const subforge::PatchValues& values);
    void handleMidi (const juce::MidiMessage& message);
    subforge::BassParams readParams() const;

    static constexpr int kOversampling = 4;

    subforge::BassSynth synth;
    juce::dsp::Oversampling<float> oversampler { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    int maxBlockSize = 512;

    juce::PropertiesFile settings;
    juce::Array<juce::File> userPresets;
    int currentEntry = 0;
    juce::String patchName { "Init" };
    int randomCount = 0;
    std::mt19937 rng { std::random_device {}() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubForgeProcessor)
};
