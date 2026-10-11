// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Patches.h"

// Sound presets: the factory patches followed by the user's .sfpreset files.
// A sound preset covers every parameter except the sequencer's (IDs starting
// with "seq"), so loading or randomizing a sound never touches the pattern or
// the sequencer settings. Runs on the message thread.
class SoundPresets
{
public:
    static constexpr const char* kExtension = ".sfpreset";

    SoundPresets (juce::AudioProcessorValueTreeState&, juce::PropertiesFile& settings, std::function<void()> onChange);

    static bool isSoundParameter (const juce::String& id) { return ! id.startsWith ("seq"); }

    // browser: factory entries first, then user files
    int numEntries() const { return subforge::kNumPresets + userFiles.size(); }
    juce::String entryName (int index) const;
    bool isUser (int index) const { return index >= subforge::kNumPresets && index < numEntries(); }
    int current() const { return currentEntry; }
    juce::String name() const { return currentName; }

    void load (int index);
    void randomize();
    void mutate();

    void rescan();
    bool exists (const juce::String& presetName) const;
    bool save (const juce::String& presetName);
    void remove (int index);

    juce::File folder() const;
    void setFolder (const juce::File& newFolder); // a default-constructed File restores the default
    static juce::File defaultFolder();

    // the current name and entry, stored with the project
    void writeState (juce::ValueTree&) const;
    void readState (const juce::ValueTree&);

private:
    subforge::PatchValues currentPatch() const;
    void applyPatch (const subforge::PatchValues&);
    bool loadFile (const juce::File&);
    void changed() { onChange(); }

    juce::AudioProcessorValueTreeState& state;
    juce::PropertiesFile& settings;
    std::function<void()> onChange;

    juce::Array<juce::File> userFiles;
    int currentEntry = 0;
    juce::String currentName { "Init" };
    int randomCount = 0;
    std::mt19937 rng { std::random_device {}() };
};
