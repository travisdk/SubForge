// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PatternLibrary.h"

// Sequencer patterns: the factory patterns followed by the user's .sfpattern
// files. A pattern covers the steps plus the groove settings (length, swing,
// rate, gate) and nothing of the sound; the root note is left alone so a
// pattern follows the key being played. Everything works on the slot that
// plays, and each slot keeps the name of what was loaded into it and its own
// groove settings. Runs on the message thread.
class PatternPresets
{
public:
    static constexpr const char* kExtension = ".sfpattern";

    PatternPresets (juce::AudioProcessorValueTreeState&, subforge::PatternBank&, std::function<juce::File()> folderProvider,
                    std::function<void()> onChange);

    // browser: factory entries first, then user files
    int numEntries() const { return subforge::kNumFactoryPatterns + userFiles.size(); }
    juce::String entryName (int index) const;
    bool isUser (int index) const { return index >= subforge::kNumFactoryPatterns && index < numEntries(); }
    int current() const { return slot().entry; }
    juce::String name() const { return slot().name; }
    juce::String slotName (int slotIndex) const { return slots[(size_t) slotIndex].name; }

    void load (int index);
    void randomize();
    void clear();
    void markEdited(); // a step was changed in the grid
    void copySlot (int from, int to);

    // A slot as a one-pass MIDI clip, "SubForge B - Chicago Jack.mid" in the MIDI
    // folder (next to the pattern folder), replacing an earlier export of the same
    // name. Returns the file, or a default File if it could not be written.
    juce::File exportMidi (int slotIndex);
    juce::File midiFolder() const { return getFolder().getParentDirectory().getChildFile ("MIDI"); }

    void rescan();
    bool exists (const juce::String& patternName) const;
    bool save (const juce::String& patternName);
    void remove (int index);
    juce::File folder() const { return getFolder(); }

    // one-line text form for sharing, e.g.
    // "SubForge pattern | length 16 | swing 0.50 | rate 1/16 | gate 0.50 | 0a 0 12s - 3a ..."
    juce::String asText() const;
    bool fromText (const juce::String& text);

    // the steps, grooves and names of all slots, stored with the project. Slot A
    // uses the properties of 1.0.x, so older versions load it; slots missing from
    // older projects start as copies of A. Projects from before 1.2.0 have no
    // groove per slot, so every slot gets the one in their sequencer parameters.
    void writeState (juce::ValueTree&) const;
    void readState (const juce::ValueTree&);

private:
    // groove settings read from a file or text; missing ones leave the slot's value
    struct LoadedGroove
    {
        int length = subforge::kMaxSteps;
        float swing = 0.5f;
        int rateIndex = -1;  // -1: leave the current rate
        float gate = -1.0f;  // < 0: leave the current gate
    };

    struct Slot
    {
        juce::String name { "Default" };
        int entry = -1;
    };

    Slot& slot() { return slots[(size_t) bank.active()]; }
    const Slot& slot() const { return slots[(size_t) bank.active()]; }
    subforge::Pattern& pattern() { return bank.activePattern(); }
    const subforge::Pattern& pattern() const { return bank.activePattern(); }
    int entryNamed (const juce::String& wanted) const;
    void applyGroove (const LoadedGroove&);
    static int rateIndexFor (const juce::String& rateName);
    void changed() { onChange(); }

    juce::AudioProcessorValueTreeState& state;
    subforge::PatternBank& bank;
    std::function<juce::File()> getFolder;
    std::function<void()> onChange;

    juce::Array<juce::File> userFiles;
    std::array<Slot, subforge::kNumPatternSlots> slots;
    int randomCount = 0;
    std::mt19937 rng { std::random_device {}() };
};
