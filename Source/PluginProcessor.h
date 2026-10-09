#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
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

    int getNumPrograms() override { return subforge::kNumPresets; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Patch handling, called from the message thread. Each one sends a change message.
    void loadPreset (int index);
    void randomizePatch();
    void mutatePatch();
    juce::String getPatchName() const { return patchName; }

    juce::AudioProcessorValueTreeState apvts;

private:
    subforge::PatchValues currentPatch() const;
    void applyPatch (const subforge::PatchValues& values);
    void handleMidi (const juce::MidiMessage& message);
    subforge::BassParams readParams() const;

    subforge::BassSynth synth;

    int currentPreset = 0;
    juce::String patchName { "Init" };
    int randomCount = 0;
    std::mt19937 rng { std::random_device {}() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubForgeProcessor)
};
