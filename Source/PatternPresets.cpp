// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PatternPresets.h"
#include "PresetFiles.h"

using namespace subforge;

namespace
{
constexpr int kDefaultRateIndex = 1; // 1/16
constexpr float kDefaultGate = 0.5f;
} // namespace

PatternPresets::PatternPresets (juce::AudioProcessorValueTreeState& s, PatternBank& b, std::function<juce::File()> folderProvider,
                                std::function<void()> callback)
    : state (s), bank (b), getFolder (std::move (folderProvider)), onChange (std::move (callback))
{
    rescan();
}

juce::String PatternPresets::entryName (int index) const
{
    if (isUser (index))
        return userFiles[index - kNumFactoryPatterns].getFileNameWithoutExtension();
    return juce::isPositiveAndBelow (index, kNumFactoryPatterns) ? kFactoryPatterns[index].name : "";
}

int PatternPresets::rateIndexFor (const juce::String& rateName)
{
    for (int i = 0; i < kNumSeqRates; ++i)
        if (rateName == kSeqRateNames[i])
            return i;
    return -1;
}

// Into the slot that plays; the processor shows it in the sequencer parameters.
void PatternPresets::applyGroove (const LoadedGroove& loaded)
{
    auto& target = pattern();
    target.setLength (loaded.length);
    target.setSwing (loaded.swing);
    if (loaded.rateIndex >= 0)
        target.setRate (loaded.rateIndex);
    if (loaded.gate >= 0.0f)
        target.setGate (loaded.gate);
}

bool PatternPresets::exists (const juce::String& patternName) const
{
    return files::fileFor (getFolder(), patternName, kExtension).existsAsFile();
}

void PatternPresets::rescan()
{
    std::array<juce::File, kNumPatternSlots> loadedFiles;
    for (size_t i = 0; i < slots.size(); ++i)
        if (isUser (slots[i].entry))
            loadedFiles[i] = userFiles[slots[i].entry - kNumFactoryPatterns];

    userFiles = files::listSorted (getFolder(), kExtension);

    for (size_t i = 0; i < slots.size(); ++i)
    {
        if (loadedFiles[i] != juce::File())
        {
            const int index = userFiles.indexOf (loadedFiles[i]);
            slots[i].entry = index >= 0 ? kNumFactoryPatterns + index : -1;
        }
    }
    changed();
}

int PatternPresets::entryNamed (const juce::String& wanted) const
{
    int found = -1;
    for (int i = 0; i < numEntries(); ++i)
        if (entryName (i) == wanted)
            found = i;
    return found;
}

void PatternPresets::load (int index)
{
    if (isUser (index))
    {
        const auto xml = files::parseXmlFile (userFiles[index - kNumFactoryPatterns]);
        if (xml == nullptr || ! xml->hasTagName ("SubForgePattern")
            || ! parseCompactPattern (xml->getStringAttribute ("steps").toRawUTF8(), pattern()))
            return;
        // files from before rate and gate were stored leave the current values
        applyGroove ({ xml->getIntAttribute ("length", kMaxSteps), (float) xml->getDoubleAttribute ("swing", 0.5),
                       rateIndexFor (xml->getStringAttribute ("rate")), (float) xml->getDoubleAttribute ("gate", -1.0) });
    }
    else if (juce::isPositiveAndBelow (index, kNumFactoryPatterns))
    {
        const auto& factory = kFactoryPatterns[index];
        parseCompactPattern (factory.steps, pattern());
        applyGroove ({ factory.length, factory.swing, kDefaultRateIndex, kDefaultGate });
    }
    else
    {
        return;
    }

    slot() = { entryName (index), index };
    changed();
}

bool PatternPresets::save (const juce::String& patternName)
{
    const auto file = files::fileFor (getFolder(), patternName, kExtension);
    if (! file.getParentDirectory().createDirectory())
        return false;

    juce::XmlElement xml ("SubForgePattern");
    xml.setAttribute ("version", 2);
    xml.setAttribute ("name", patternName.trim());
    const auto groove = pattern().groove();
    xml.setAttribute ("length", groove.length);
    xml.setAttribute ("swing", groove.swing);
    xml.setAttribute ("rate", kSeqRateNames[groove.rate]);
    xml.setAttribute ("gate", groove.gate);
    xml.setAttribute ("steps", juce::String (toCompactPattern (pattern())));
    if (! xml.writeTo (file))
        return false;

    slot() = { file.getFileNameWithoutExtension(), -1 };
    rescan();
    slot().entry = kNumFactoryPatterns + userFiles.indexOf (file);
    changed();
    return true;
}

void PatternPresets::remove (int index)
{
    if (! isUser (index))
        return;

    files::moveToTrash (userFiles[index - kNumFactoryPatterns]);
    for (auto& s : slots)
        if (s.entry == index)
            s.entry = -1;
    rescan();
}

void PatternPresets::randomize()
{
    pattern().randomize (rng);
    slot() = { "Random " + juce::String (++randomCount), -1 };
    changed();
}

void PatternPresets::clear()
{
    pattern().clear();
    slot() = { "Empty", -1 };
    changed();
}

void PatternPresets::markEdited()
{
    if (! slot().name.endsWith ("*"))
    {
        slot().name << " *";
        changed();
    }
}

void PatternPresets::copySlot (int from, int to)
{
    if (! juce::isPositiveAndBelow (from, kNumPatternSlots) || ! juce::isPositiveAndBelow (to, kNumPatternSlots) || from == to)
        return;
    bank.slot (to).copyFrom (bank.slot (from));
    slots[(size_t) to] = slots[(size_t) from];
    changed();
}

juce::File PatternPresets::exportMidi (int slotIndex)
{
    if (! juce::isPositiveAndBelow (slotIndex, kNumPatternSlots))
        return {};

    constexpr int ticksPerQuarter = 960;
    const auto clip = patternToClip (bank.slot (slotIndex), (int) state.getRawParameterValue ("seqRoot")->load());

    const auto name = "SubForge " + juce::String::charToString ((juce::juce_wchar) ('A' + slotIndex)) + " - "
                      + slots[(size_t) slotIndex].name.trimCharactersAtEnd (" *");
    juce::MidiMessageSequence sequence;
    sequence.addEvent (juce::MidiMessage::textMetaEvent (3, name)); // track name
    for (const auto& n : clip.notes)
    {
        sequence.addEvent (juce::MidiMessage::noteOn (1, n.note, (juce::uint8) n.velocity).withTimeStamp (n.start * ticksPerQuarter));
        sequence.addEvent (juce::MidiMessage::noteOff (1, n.note).withTimeStamp (n.end * ticksPerQuarter));
    }
    sequence.updateMatchedPairs();
    // the track ends with the pattern, so the clip is as long as one pass
    sequence.addEvent (juce::MidiMessage::endOfTrack().withTimeStamp (clip.lengthBeats * ticksPerQuarter));

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote (ticksPerQuarter);
    midiFile.addTrack (sequence);

    const auto file = files::fileFor (midiFolder(), name, ".mid");
    if (! midiFolder().createDirectory())
        return {};
    juce::FileOutputStream stream (file);
    if (! stream.openedOk())
        return {};
    stream.setPosition (0);
    stream.truncate();
    return midiFile.writeTo (stream) ? file : juce::File();
}

juce::String PatternPresets::asText() const
{
    const auto groove = pattern().groove();
    return "SubForge pattern | length " + juce::String (groove.length)
           + " | swing " + juce::String (groove.swing, 2)
           + " | rate " + kSeqRateNames[groove.rate]
           + " | gate " + juce::String (groove.gate, 2)
           + " | " + juce::String (toCompactPattern (pattern()));
}

// Accepts the line above (any of its fields may be missing), or just the sixteen step tokens.
bool PatternPresets::fromText (const juce::String& text)
{
    auto parts = juce::StringArray::fromTokens (text.trim(), "|", "");
    parts.trim();
    if (parts.isEmpty() || ! parseCompactPattern (parts[parts.size() - 1].toRawUTF8(), pattern()))
        return false;

    LoadedGroove groove;
    for (const auto& part : parts)
    {
        const auto value = part.fromFirstOccurrenceOf (" ", false, false).trim();
        if (part.startsWithIgnoreCase ("length"))
            groove.length = value.getIntValue();
        else if (part.startsWithIgnoreCase ("swing"))
            groove.swing = value.getFloatValue();
        else if (part.startsWithIgnoreCase ("rate"))
            groove.rateIndex = rateIndexFor (value);
        else if (part.startsWithIgnoreCase ("gate"))
            groove.gate = value.getFloatValue();
    }
    applyGroove (groove);
    slot() = { "Pasted", -1 };
    changed();
    return true;
}

// Slot A: "pattern" and "patternName" as in 1.0.x; B-H: "patternB", "patternNameB" ...
static juce::String slotSuffix (int index)
{
    return index == 0 ? juce::String() : juce::String::charToString ((juce::juce_wchar) ('A' + index));
}

void PatternPresets::writeState (juce::ValueTree& tree) const
{
    for (int i = 0; i < kNumPatternSlots; ++i)
    {
        tree.setProperty ("pattern" + slotSuffix (i), juce::String (bank.slot (i).toString()), nullptr);
        tree.setProperty ("patternName" + slotSuffix (i), slots[(size_t) i].name, nullptr);
        tree.setProperty ("patternGroove" + slotSuffix (i), juce::String (bank.slot (i).groove().toString()), nullptr);
    }
}

void PatternPresets::readState (const juce::ValueTree& tree)
{
    for (int i = 0; i < kNumPatternSlots; ++i)
    {
        const auto steps = "pattern" + slotSuffix (i), name = "patternName" + slotSuffix (i);
        auto& target = bank.slot (i);
        if (i > 0 && ! tree.hasProperty (steps)) // a project from 1.0.x
        {
            target.copyFrom (bank.slot (0));
            slots[(size_t) i] = slots[0];
            continue;
        }
        if (tree.hasProperty (steps))
            target.fromString (tree.getProperty (steps).toString().toStdString());
        const auto slotName = tree.getProperty (name, "Default").toString();
        slots[(size_t) i] = { slotName, entryNamed (slotName) };
    }

    // the shared groove of a project from before 1.2.0, as its parameters held it
    auto saved = [&] (const char* id)
    {
        const auto param = tree.getChildWithProperty ("id", id);
        return param.isValid() ? (float) param.getProperty ("value") : state.getRawParameterValue (id)->load();
    };
    const Groove shared { juce::roundToInt (saved ("seqLength")), juce::roundToInt (saved ("seqRate")), saved ("seqSwing"),
                          saved ("seqGate") };
    std::array<std::string, kNumPatternSlots> grooves;
    for (int i = 0; i < kNumPatternSlots; ++i)
        grooves[(size_t) i] = tree.getProperty ("patternGroove" + slotSuffix (i)).toString().toStdString();
    restoreGrooves (bank, grooves, shared);
    changed();
}
