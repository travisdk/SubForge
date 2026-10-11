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

PatternPresets::PatternPresets (juce::AudioProcessorValueTreeState& s, Pattern& p, std::function<juce::File()> folderProvider,
                                std::function<void()> callback)
    : state (s), pattern (p), getFolder (std::move (folderProvider)), onChange (std::move (callback))
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

void PatternPresets::applyGroove (const Groove& groove)
{
    files::setParameterValue (*state.getParameter ("seqLength"), (float) juce::jlimit (1, kMaxSteps, groove.length));
    files::setParameterValue (*state.getParameter ("seqSwing"), juce::jlimit (0.5f, 0.75f, groove.swing));
    if (groove.rateIndex >= 0)
        files::setParameterValue (*state.getParameter ("seqRate"), (float) juce::jlimit (0, kNumSeqRates - 1, groove.rateIndex));
    if (groove.gate >= 0.0f)
        files::setParameterValue (*state.getParameter ("seqGate"), juce::jlimit (0.05f, 1.0f, groove.gate));
}

bool PatternPresets::exists (const juce::String& patternName) const
{
    return files::fileFor (getFolder(), patternName, kExtension).existsAsFile();
}

void PatternPresets::rescan()
{
    const auto currentFile = isUser (currentEntry) ? userFiles[currentEntry - kNumFactoryPatterns] : juce::File();
    userFiles = files::listSorted (getFolder(), kExtension);

    if (currentFile != juce::File())
    {
        const int index = userFiles.indexOf (currentFile);
        currentEntry = index >= 0 ? kNumFactoryPatterns + index : -1;
    }
    changed();
}

void PatternPresets::load (int index)
{
    if (isUser (index))
    {
        const auto xml = juce::parseXML (userFiles[index - kNumFactoryPatterns]);
        if (xml == nullptr || ! xml->hasTagName ("SubForgePattern")
            || ! parseCompactPattern (xml->getStringAttribute ("steps").toRawUTF8(), pattern))
            return;
        // files from before rate and gate were stored leave the current values
        applyGroove ({ xml->getIntAttribute ("length", kMaxSteps), (float) xml->getDoubleAttribute ("swing", 0.5),
                       rateIndexFor (xml->getStringAttribute ("rate")), (float) xml->getDoubleAttribute ("gate", -1.0) });
    }
    else if (juce::isPositiveAndBelow (index, kNumFactoryPatterns))
    {
        const auto& factory = kFactoryPatterns[index];
        parseCompactPattern (factory.steps, pattern);
        applyGroove ({ factory.length, factory.swing, kDefaultRateIndex, kDefaultGate });
    }
    else
    {
        return;
    }

    currentEntry = index;
    currentName = entryName (index);
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
    xml.setAttribute ("length", (int) state.getRawParameterValue ("seqLength")->load());
    xml.setAttribute ("swing", state.getRawParameterValue ("seqSwing")->load());
    xml.setAttribute ("rate", kSeqRateNames[juce::jlimit (0, kNumSeqRates - 1, (int) state.getRawParameterValue ("seqRate")->load())]);
    xml.setAttribute ("gate", state.getRawParameterValue ("seqGate")->load());
    xml.setAttribute ("steps", juce::String (toCompactPattern (pattern)));
    if (! xml.writeTo (file))
        return false;

    currentName = file.getFileNameWithoutExtension();
    currentEntry = -1;
    rescan();
    currentEntry = kNumFactoryPatterns + userFiles.indexOf (file);
    changed();
    return true;
}

void PatternPresets::remove (int index)
{
    if (! isUser (index))
        return;

    files::moveToTrash (userFiles[index - kNumFactoryPatterns]);
    if (currentEntry == index)
        currentEntry = -1;
    rescan();
}

void PatternPresets::randomize()
{
    pattern.randomize (rng);
    currentName = "Random " + juce::String (++randomCount);
    changed();
}

void PatternPresets::clear()
{
    pattern.clear();
    currentName = "Empty";
    changed();
}

void PatternPresets::markEdited()
{
    if (! currentName.endsWith ("*"))
    {
        currentName << " *";
        changed();
    }
}

juce::String PatternPresets::asText() const
{
    const int rate = juce::jlimit (0, kNumSeqRates - 1, (int) state.getRawParameterValue ("seqRate")->load());
    return "SubForge pattern | length " + juce::String ((int) state.getRawParameterValue ("seqLength")->load())
           + " | swing " + juce::String (state.getRawParameterValue ("seqSwing")->load(), 2)
           + " | rate " + kSeqRateNames[rate]
           + " | gate " + juce::String (state.getRawParameterValue ("seqGate")->load(), 2)
           + " | " + juce::String (toCompactPattern (pattern));
}

// Accepts the line above (any of its fields may be missing), or just the sixteen step tokens.
bool PatternPresets::fromText (const juce::String& text)
{
    auto parts = juce::StringArray::fromTokens (text.trim(), "|", "");
    parts.trim();
    if (parts.isEmpty() || ! parseCompactPattern (parts[parts.size() - 1].toRawUTF8(), pattern))
        return false;

    Groove groove;
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
    currentName = "Pasted";
    currentEntry = -1;
    changed();
    return true;
}

void PatternPresets::writeState (juce::ValueTree& tree) const
{
    tree.setProperty ("pattern", juce::String (pattern.toString()), nullptr);
    tree.setProperty ("patternName", currentName, nullptr);
}

void PatternPresets::readState (const juce::ValueTree& tree)
{
    if (tree.hasProperty ("pattern"))
        pattern.fromString (tree.getProperty ("pattern").toString().toStdString());
    currentName = tree.getProperty ("patternName", "Default").toString();
    currentEntry = -1;
    for (int i = 0; i < numEntries(); ++i)
        if (entryName (i) == currentName)
            currentEntry = i;
    changed();
}
