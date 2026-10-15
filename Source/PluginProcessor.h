// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Patches.h"
#include "Sequencer.h"
#include "PatternLibrary.h"
#include "SoundPresets.h"
#include "PatternPresets.h"

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

    // Host programs are the factory sound presets only.
    int getNumPrograms() override { return subforge::kNumPresets; }
    int getCurrentProgram() override { return soundPresets.isUser (soundPresets.current()) ? 0 : soundPresets.current(); }
    void setCurrentProgram (int index) override { soundPresets.load (index); }
    const juce::String getProgramName (int index) override { return soundPresets.entryName (index); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // The two kinds of presets are independent: a sound preset never touches the
    // sequencer and a pattern never touches the sound. Both send a change message
    // from this processor when something changes.
    SoundPresets& getSoundPresets() { return soundPresets; }
    PatternPresets& getPatternPresets() { return patternPresets; }

    // Step sequencer pattern slots, edited by the GUI while the audio thread plays them.
    // The GUI edits the slot that plays.
    subforge::PatternBank& getPatternBank() { return patternBank; }
    subforge::Pattern& getPattern() { return patternBank.activePattern(); }
    int getSequencerStep() const { return sequencer.getCurrentStep(); }

    // A slot chosen in the GUI: sets the Pattern parameter (so hosts can record it) and
    // switches at once while the sequencer isn't running.
    void choosePatternSlot (int slot);

    // MIDI notes 0-7 choose the slot while the sequencer runs (a global setting, on by default).
    bool slotNotesEnabled() const { return slotNotes.load(); }
    void setSlotNotesEnabled (bool enabled);

    // Global preferences shared by all instances (accent colour, UI size, preset folder).
    juce::PropertiesFile& getSettings() { return settings; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void handleMidi (const juce::MidiMessage& message);
    void buildVoiceMidi (const juce::MidiBuffer& midi, int numSamples, double bpm, double ppq, bool playing);
    subforge::BassParams readParams() const;

    static constexpr int kOversampling = 4;

    // Saved with the plug-in state so later versions can migrate older projects.
    // State without a number was saved by 1.0.0 and counts as version 1.
    //   1: 1.0.0
    //   2: 1.0.1, distortion auto-gain recalibrated (no migration: old projects play louder)
    static constexpr int kStateVersion = 2;

    subforge::BassSynth synth;
    subforge::PatternBank patternBank;
    subforge::SequencerEngine sequencer;
    std::atomic<int> lastPatternParam { 0 }; // a change of the Pattern parameter asks for its slot
    std::atomic<bool> sequencerRunning { false };
    std::atomic<bool> slotNotes { true };
    juce::MidiBuffer voiceMidi;              // incoming MIDI merged with sequencer notes
    std::array<int, 16> transposeKeys {};    // keys held while the sequencer runs
    int numTransposeKeys = 0;
    juce::dsp::Oversampling<float> oversampler { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    int maxBlockSize = 512;

    juce::PropertiesFile settings;
    SoundPresets soundPresets;     // declared after settings and patternBank, which they use
    PatternPresets patternPresets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubForgeProcessor)
};
