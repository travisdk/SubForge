// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// File and parameter helpers shared by SoundPresets and PatternPresets.
namespace subforge::files
{

// The user's files with the given extension in a folder, sorted by name.
inline juce::Array<juce::File> listSorted (const juce::File& folder, const char* extension)
{
    auto files = folder.findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension);
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0; });
    return files;
}

inline juce::File fileFor (const juce::File& folder, const juce::String& name, const char* extension)
{
    return folder.getChildFile (juce::File::createLegalFileName (name.trim()) + extension);
}

// Sets a parameter as one host-visible gesture.
inline void setParameterValue (juce::RangedAudioParameter& param, float value)
{
    param.beginChangeGesture();
    param.setValueNotifyingHost (param.convertTo0to1 (value));
    param.endChangeGesture();
}

// JUCE's Linux File::moveToTrash() skips the .trashinfo file, so file managers
// can't show or restore the file. This follows the freedesktop.org trash spec.
inline bool moveToTrash (const juce::File& file)
{
   #if JUCE_LINUX
    const auto dataHome = juce::SystemStats::getEnvironmentVariable ("XDG_DATA_HOME", {});
    const auto trash = (dataHome.isNotEmpty() ? juce::File (dataHome)
                                              : juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                                    .getChildFile (".local/share"))
                           .getChildFile ("Trash");
    const auto files = trash.getChildFile ("files"), info = trash.getChildFile ("info");

    if (files.createDirectory() && info.createDirectory())
    {
        const auto target = files.getNonexistentChildFile (file.getFileNameWithoutExtension(), file.getFileExtension(), false);
        const auto infoFile = info.getChildFile (target.getFileName() + ".trashinfo");
        auto segments = juce::StringArray::fromTokens (file.getFullPathName(), "/", {});
        for (auto& segment : segments)
            segment = juce::URL::addEscapeChars (segment, false); // escape the parts, keep the slashes
        const auto infoText = "[Trash Info]\nPath=" + segments.joinIntoString ("/")
                              + "\nDeletionDate=" + juce::Time::getCurrentTime().formatted ("%Y-%m-%dT%H:%M:%S") + "\n";

        if (infoFile.replaceWithText (infoText))
        {
            if (file.moveFileTo (target))
                return true;
            infoFile.deleteFile();
        }
    }
   #endif
    return file.moveToTrash();
}

} // namespace subforge::files
