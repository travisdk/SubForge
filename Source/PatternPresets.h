// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PatternLibrary.h"

// Sequencer patterns: the factory patterns followed by the user's .sfpattern
// files. A pattern covers the steps plus the groove settings (length, swing,
// rate, gate) and nothing of the sound; the root note is left alone so a
// pattern follows the key being played. Runs on the message thread.
class PatternPresets
{
public:
    static constexpr const char* kExtension = ".sfpattern";

    PatternPresets (juce::AudioProcessorValueTreeState&, subforge::Pattern&, std::function<juce::File()> folderProvider,
                    std::function<void()> onChange);

    // browser: factory entries first, then user files
    int numEntries() const { return subforge::kNumFactoryPatterns + userFiles.size(); }
    juce::String entryName (int index) const;
    bool isUser (int index) const { return index >= subforge::kNumFactoryPatterns && index < numEntries(); }
    int current() const { return currentEntry; }
    juce::String name() const { return currentName; }

    void load (int index);
    void randomize();
    void clear();
    void markEdited(); // a step was changed in the grid

    void rescan();
    bool exists (const juce::String& patternName) const;
    bool save (const juce::String& patternName);
    void remove (int index);
    juce::File folder() const { return getFolder(); }

    // one-line text form for sharing, e.g.
    // "SubForge pattern | length 16 | swing 0.50 | rate 1/16 | gate 0.50 | 0a 0 12s - 3a ..."
    juce::String asText() const;
    bool fromText (const juce::String& text);

    // the steps, current name and entry, stored with the project
    void writeState (juce::ValueTree&) const;
    void readState (const juce::ValueTree&);

private:
    struct Groove
    {
        int length = subforge::kMaxSteps;
        float swing = 0.5f;
        int rateIndex = -1;  // -1: leave the current rate
        float gate = -1.0f;  // < 0: leave the current gate
    };

    void applyGroove (const Groove&);
    static int rateIndexFor (const juce::String& rateName);
    void changed() { onChange(); }

    juce::AudioProcessorValueTreeState& state;
    subforge::Pattern& pattern;
    std::function<juce::File()> getFolder;
    std::function<void()> onChange;

    juce::Array<juce::File> userFiles;
    int currentEntry = -1;
    juce::String currentName { "Default" };
    int randomCount = 0;
    std::mt19937 rng { std::random_device {}() };
};
