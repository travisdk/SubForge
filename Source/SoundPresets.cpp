// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#include "SoundPresets.h"
#include "PresetFiles.h"

using namespace subforge;

SoundPresets::SoundPresets (juce::AudioProcessorValueTreeState& s, juce::PropertiesFile& prefs, std::function<void()> callback)
    : state (s), settings (prefs), onChange (std::move (callback))
{
    rescan();
}

juce::String SoundPresets::entryName (int index) const
{
    if (isUser (index))
        return userFiles[index - kNumPresets].getFileNameWithoutExtension();
    return juce::isPositiveAndBelow (index, kNumPresets) ? kPresets[index].name : "";
}

juce::File SoundPresets::defaultFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("SubForge/Presets");
}

juce::File SoundPresets::folder() const
{
    const auto custom = settings.getValue ("presetFolder");
    return custom.isNotEmpty() ? juce::File (custom) : defaultFolder();
}

void SoundPresets::setFolder (const juce::File& newFolder)
{
    settings.setValue ("presetFolder", newFolder == juce::File() ? juce::String() : newFolder.getFullPathName());
    settings.saveIfNeeded();
    if (isUser (currentEntry))
        currentEntry = 0;
    rescan();
}

bool SoundPresets::exists (const juce::String& presetName) const
{
    return files::fileFor (folder(), presetName, kExtension).existsAsFile();
}

void SoundPresets::rescan()
{
    const auto currentFile = isUser (currentEntry) ? userFiles[currentEntry - kNumPresets] : juce::File();
    userFiles = files::listSorted (folder(), kExtension);

    if (currentFile != juce::File())
    {
        const int index = userFiles.indexOf (currentFile);
        currentEntry = index >= 0 ? kNumPresets + index : 0;
    }
    changed();
}

// Stores every sound parameter as a plain value, including volume: unlike
// factory presets a user preset is restored exactly as it was saved.
bool SoundPresets::save (const juce::String& presetName)
{
    const auto file = files::fileFor (folder(), presetName, kExtension);
    if (file.getFileName() == kExtension) // nothing of the name is legal in a file name
        return false;
    if (! file.getParentDirectory().createDirectory())
        return false;

    juce::XmlElement xml ("SubForgePreset");
    xml.setAttribute ("version", 2);
    xml.setAttribute ("name", presetName.trim());
    for (auto* p : state.processor.getParameters())
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (p); param != nullptr && isSoundParameter (param->paramID))
        {
            auto* element = xml.createNewChildElement ("Param");
            element->setAttribute ("id", param->paramID);
            element->setAttribute ("value", param->convertFrom0to1 (param->getValue()));
        }

    if (! xml.writeTo (file))
        return false;

    currentName = file.getFileNameWithoutExtension();
    currentEntry = -1;
    rescan();
    currentEntry = kNumPresets + userFiles.indexOf (file);
    changed();
    return true;
}

// Sound parameters missing from older files, or not numbers in a damaged one, fall
// back to their defaults. Sequencer values and patterns that version-1 files contain
// are ignored.
bool SoundPresets::loadFile (const juce::File& file)
{
    const auto xml = files::parseXmlFile (file);
    if (xml == nullptr || ! xml->hasTagName ("SubForgePreset"))
        return false;

    for (auto* p : state.processor.getParameters())
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (p); param != nullptr && isSoundParameter (param->paramID))
        {
            float value = param->convertFrom0to1 (param->getDefaultValue());
            if (auto* element = xml->getChildByAttribute ("id", param->paramID))
                if (const auto loaded = (float) element->getDoubleAttribute ("value", value); ! std::isnan (loaded))
                    value = loaded;
            files::setParameterValue (*param, value);
        }
    return true;
}

void SoundPresets::remove (int index)
{
    if (! isUser (index))
        return;

    files::moveToTrash (userFiles[index - kNumPresets]);
    if (currentEntry == index)
        currentEntry = 0;
    rescan();
}

PatchValues SoundPresets::currentPatch() const
{
    PatchValues values {};
    for (int i = 0; i < kNumPatchParams; ++i)
        values[(size_t) i] = state.getRawParameterValue (kPatchParamIds[i])->load();
    return values;
}

void SoundPresets::applyPatch (const PatchValues& values)
{
    for (int i = 0; i < kNumPatchParams; ++i)
        files::setParameterValue (*state.getParameter (kPatchParamIds[i]), values[(size_t) i]);
    files::setParameterValue (*state.getParameter ("volume"), normalizedVolumeDb (values));
}

void SoundPresets::load (int index)
{
    if (isUser (index))
    {
        if (! loadFile (userFiles[index - kNumPresets]))
            return;
    }
    else if (juce::isPositiveAndBelow (index, kNumPresets))
    {
        applyPatch (kPresets[index].values());
    }
    else
    {
        return;
    }

    currentEntry = index;
    currentName = entryName (index);
    changed();
}

void SoundPresets::randomize()
{
    applyPatch (randomPatch (rng));
    currentName = "Random " + juce::String (++randomCount);
    changed();
}

void SoundPresets::mutate()
{
    std::uniform_real_distribution<float> uniform (0.0f, 1.0f);
    std::normal_distribution<float> nudge (0.0f, 0.07f);

    auto values = currentPatch();
    for (int i = 0; i < kNumPatchParams; ++i)
    {
        auto* param = state.getParameter (kPatchParamIds[i]);
        auto& value = values[(size_t) i];

        if (i == pSquareWidth && values[(size_t) pWave] != 1.0f) // only a square wave has a width
            continue;

        if (dynamic_cast<juce::AudioParameterChoice*> (param) != nullptr
            || dynamic_cast<juce::AudioParameterInt*> (param) != nullptr
            || dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            // discrete choices only flip occasionally
            if (uniform (rng) < 0.1f)
                value = param->convertFrom0to1 (uniform (rng));
            value = std::round (value);
        }
        else
        {
            const float normalised = juce::jlimit (0.0f, 1.0f, param->convertTo0to1 (value) + nudge (rng));
            value = param->convertFrom0to1 (normalised);
        }
    }

    applyPatch (values);
    if (! currentName.endsWith ("*"))
        currentName << " *";
    changed();
}

void SoundPresets::writeState (juce::ValueTree& tree) const
{
    tree.setProperty ("patchName", currentName, nullptr);
    tree.setProperty ("presetIndex", isUser (currentEntry) ? 0 : currentEntry, nullptr);
}

void SoundPresets::readState (const juce::ValueTree& tree)
{
    currentName = tree.getProperty ("patchName", "Init").toString();
    currentEntry = juce::jlimit (0, kNumPresets - 1, (int) tree.getProperty ("presetIndex", 0));
    for (int i = kNumPresets; i < numEntries(); ++i)
        if (entryName (i) == currentName)
            currentEntry = i; // a user preset of that name is the better match
    changed();
}
