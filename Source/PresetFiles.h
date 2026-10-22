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

// JUCE's XML parser goes one level deeper on the stack for each nested element, so a
// damaged or hostile file nested tens of thousands deep crashes the host, and a huge one
// stalls it. Real files are a few kB with about fifty elements; far beyond that is refused.
constexpr size_t kMaxXmlBytes = 1 << 20;
constexpr int kMaxXmlElements = 1000; // each element starts with '<', so this also limits the nesting

inline bool isPlausibleXml (const void* data, size_t numBytes)
{
    const auto* text = static_cast<const char*> (data);
    return numBytes <= kMaxXmlBytes && std::count (text, text + numBytes, '<') <= kMaxXmlElements;
}

// juce::parseXML for preset and pattern files, or nullptr if the file is not plausible.
inline std::unique_ptr<juce::XmlElement> parseXmlFile (const juce::File& file)
{
    juce::MemoryBlock data;
    if (file.getSize() > (juce::int64) kMaxXmlBytes || ! file.loadFileAsData (data) || ! isPlausibleXml (data.getData(), data.getSize()))
        return nullptr;
    return juce::parseXML (juce::String::createStringFromData (data.getData(), (int) data.getSize()));
}

// Sets a parameter as one host-visible gesture.
inline void setParameterValue (juce::RangedAudioParameter& param, float value)
{
    param.beginChangeGesture();
    param.setValueNotifyingHost (param.convertTo0to1 (value));
    param.endChangeGesture();
}

// Shows a folder in the file manager. On Linux, JUCE's File::startAsProcess() hands the
// path to a shell, which runs "$(...)" in a folder name; here xdg-open gets it as an argument.
inline bool openFolder (const juce::File& folder)
{
   #if JUCE_LINUX
    juce::ChildProcess opener; // xdg-open keeps running after this goes
    return folder.isDirectory() && opener.start (juce::StringArray { "xdg-open", folder.getFullPathName() }, 0);
   #else
    return folder.startAsProcess();
   #endif
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
