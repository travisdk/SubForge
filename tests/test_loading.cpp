// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Damaged or hostile projects, preset and pattern files, loaded through the real
// processor (this test links JUCE). Files go to a temporary folder, never the user's.
#include "PluginProcessor.h"
#include "PresetFiles.h"
#include "TestUtils.h"

#include <cmath>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

struct Transport : juce::AudioPlayHead
{
    double ppq = 0.0;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm (120.0);
        info.setPpqPosition (ppq);
        info.setIsPlaying (true);
        return info;
    }
};

// Two seconds with the sequencer on and a note held; true if every sample is finite.
bool rendersFinite (SubForgeProcessor& processor)
{
    Transport transport;
    processor.setPlayHead (&transport);
    processor.setRateAndBufferSizeDetails (kSampleRate, kBlockSize);
    processor.prepareToPlay (kSampleRate, kBlockSize);
    if (auto* seqOn = processor.apvts.getParameter ("seqOn"))
        seqOn->setValueNotifyingHost (1.0f);

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    bool finite = true;
    for (int block = 0; block * kBlockSize < 2 * kSampleRate; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36, 0.9f), 0);
        processor.processBlock (buffer, midi);
        transport.ppq += kBlockSize / kSampleRate * 2.0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                finite = finite && std::isfinite (buffer.getSample (ch, i));
    }
    processor.setPlayHead (nullptr);
    return finite;
}

bool parametersFinite (SubForgeProcessor& processor)
{
    for (auto* param : processor.getParameters())
        if (! std::isfinite (param->getValue()))
            return false;
    return true;
}

float plainValue (SubForgeProcessor& processor, const char* id)
{
    return processor.apvts.getRawParameterValue (id)->load();
}

float plainDefault (SubForgeProcessor& processor, const char* id)
{
    auto* param = processor.apvts.getParameter (id);
    return param->convertFrom0to1 (param->getDefaultValue());
}

void setState (SubForgeProcessor& processor, const juce::XmlElement& xml)
{
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary (xml, data);
    processor.setStateInformation (data.getData(), (int) data.getSize());
}

std::unique_ptr<juce::XmlElement> savedState (SubForgeProcessor& processor)
{
    juce::MemoryBlock data;
    processor.getStateInformation (data);
    return juce::AudioProcessor::getXmlFromBinary (data.getData(), (int) data.getSize());
}

const char* const kNotNumbers[] = { "nan", "-nan", "NaN" };
} // namespace

int main()
{
    const juce::ScopedJuceInitialiser_GUI juceInit;

    const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getNonexistentChildFile ("SubForgeTest", {}, false);
    CHECK (root.createDirectory());
    juce::PropertiesFile prefs (root.getChildFile ("test.settings"), {});

    // a project whose parameters are not numbers loads with their defaults and plays
    for (auto* text : kNotNumbers)
    {
        SubForgeProcessor processor;
        auto xml = savedState (processor);
        CHECK (xml != nullptr);
        for (auto* param : xml->getChildIterator())
            param->setAttribute ("value", text);
        xml->setAttribute ("patternGroove", "length 16 rate 1/16 swing nan gate nan");
        setState (processor, *xml);
        CHECK (parametersFinite (processor));
        CHECK (juce::approximatelyEqual (plainValue (processor, "cutoff"), plainDefault (processor, "cutoff")));
        CHECK (rendersFinite (processor));
    }

    // the same in a sound preset file
    for (auto* text : kNotNumbers)
    {
        SubForgeProcessor processor;
        SoundPresets presets (processor.apvts, prefs, [] {});
        presets.setFolder (root.getChildFile ("Presets"));
        juce::XmlElement xml ("SubForgePreset");
        for (auto* param : processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            {
                auto* element = xml.createNewChildElement ("Param");
                element->setAttribute ("id", ranged->paramID);
                element->setAttribute ("value", text);
            }
        CHECK (root.getChildFile ("Presets").createDirectory());
        CHECK (xml.writeTo (root.getChildFile ("Presets/Broken.sfpreset")));
        presets.rescan();
        presets.load (subforge::kNumPresets);
        CHECK (presets.name() == "Broken");
        CHECK (parametersFinite (processor));
        CHECK (juce::approximatelyEqual (plainValue (processor, "cutoff"), plainDefault (processor, "cutoff")));
        CHECK (rendersFinite (processor));
    }

    // swing and gate that are not numbers, from a pattern file or pasted text, leave the defaults
    {
        SubForgeProcessor processor;
        PatternPresets patterns (processor.apvts, processor.getPatternBank(), [root] { return root.getChildFile ("Patterns"); },
                                 [] {});
        CHECK (root.getChildFile ("Patterns").createDirectory());
        CHECK (root.getChildFile ("Patterns/Broken.sfpattern")
                   .replaceWithText ("<SubForgePattern length=\"16\" swing=\"nan\" rate=\"1/16\" gate=\"nan\""
                                     " steps=\"0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\"/>"));
        patterns.rescan();
        patterns.load (subforge::kNumFactoryPatterns);
        CHECK (patterns.name() == "Broken");
        CHECK (std::isfinite (processor.getPattern().groove().swing));
        CHECK (std::isfinite (processor.getPattern().groove().gate));
        CHECK (rendersFinite (processor));

        CHECK (patterns.fromText ("SubForge pattern | swing nan | gate nan | 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"));
        CHECK (std::isfinite (processor.getPattern().groove().swing));
        CHECK (std::isfinite (processor.getPattern().groove().gate));
        CHECK (rendersFinite (processor));
    }

    // Elements nested far deeper than any real file would overflow the stack in JUCE's
    // parser, and a huge file would stall the host: both are refused and change nothing.
    {
        constexpr int deep = 100000;
        const auto nested = juce::String::repeatedString ("<a>", deep) + juce::String::repeatedString ("</a>", deep);

        SubForgeProcessor processor;
        auto xml = savedState (processor);
        juce::MemoryOutputStream deepState;
        deepState.writeInt (0x21324356); // copyXmlToBinary's format: magic, size, text
        const auto text = "<PARAMS>" + nested + "</PARAMS>";
        deepState.writeInt ((int) text.getNumBytesAsUTF8() + 1);
        deepState.write (text.toRawUTF8(), text.getNumBytesAsUTF8() + 1);
        processor.setStateInformation (deepState.getData(), (int) deepState.getDataSize());

        xml->setAttribute ("patternName", juce::String::repeatedString ("x", 2 << 20));
        setState (processor, *xml);
        CHECK (processor.getPatternPresets().name() == "Default");

        SoundPresets presets (processor.apvts, prefs, [] {});
        presets.setFolder (root.getChildFile ("Presets"));
        CHECK (root.getChildFile ("Presets/Deep.sfpreset").replaceWithText ("<SubForgePreset>" + nested + "</SubForgePreset>"));
        CHECK (root.getChildFile ("Presets/Huge.sfpreset")
                   .replaceWithText ("<SubForgePreset name=\"" + juce::String::repeatedString ("x", 2 << 20) + "\"/>"));
        presets.rescan();
        for (int i = subforge::kNumPresets; i < presets.numEntries(); ++i)
            if (presets.entryName (i) != "Broken")
            {
                presets.load (i);
                CHECK (presets.current() != i);
            }

        PatternPresets patterns (processor.apvts, processor.getPatternBank(), [root] { return root.getChildFile ("Patterns"); },
                                 [] {});
        CHECK (root.getChildFile ("Patterns/Deep.sfpattern").replaceWithText ("<SubForgePattern>" + nested + "</SubForgePattern>"));
        patterns.rescan();
        for (int i = subforge::kNumFactoryPatterns; i < patterns.numEntries(); ++i)
            if (patterns.entryName (i) == "Deep")
                patterns.load (i);
        CHECK (patterns.name() != "Deep");
    }

    // a preset or project from before Square Width existed loads with 50 %, not the width left over
    {
        SubForgeProcessor processor;
        SoundPresets presets (processor.apvts, prefs, [] {});
        presets.setFolder (root.getChildFile ("Presets"));
        auto setWidth = [&processor] (float value)
        {
            subforge::files::setParameterValue (*processor.apvts.getParameter ("squareWidth"), value);
        };

        setWidth (0.2f);
        presets.load (0);
        CHECK (juce::approximatelyEqual (plainValue (processor, "squareWidth"), 0.5f)); // a factory preset

        juce::XmlElement preset ("SubForgePreset");
        auto* cutoff = preset.createNewChildElement ("Param");
        cutoff->setAttribute ("id", "cutoff");
        cutoff->setAttribute ("value", 500.0);
        CHECK (root.getChildFile ("Presets").createDirectory());
        CHECK (preset.writeTo (root.getChildFile ("Presets/Old.sfpreset")));
        presets.rescan();
        setWidth (0.2f);
        for (int i = subforge::kNumPresets; i < presets.numEntries(); ++i)
            if (presets.entryName (i) == "Old")
                presets.load (i);
        CHECK (presets.name() == "Old");
        CHECK (juce::approximatelyEqual (plainValue (processor, "squareWidth"), 0.5f));

        auto oldProject = savedState (processor);
        CHECK (oldProject != nullptr);
        if (auto* child = oldProject->getChildByAttribute ("id", "squareWidth"))
            oldProject->removeChildElement (child, true);
        CHECK (oldProject->getChildByAttribute ("id", "squareWidth") == nullptr);
        setWidth (0.2f);
        setState (processor, *oldProject);
        CHECK (juce::approximatelyEqual (plainValue (processor, "squareWidth"), 0.5f));
        CHECK (std::abs (plainValue (processor, "cutoff") - 500.0f) < 0.5f);

        // a saved project keeps its width
        setWidth (0.2f);
        auto project = savedState (processor);
        setWidth (0.7f);
        setState (processor, *project);
        CHECK (std::abs (plainValue (processor, "squareWidth") - 0.2f) < 1.0e-3f);
    }

    // Mutate leaves the width alone unless the wave is a square
    {
        SubForgeProcessor processor;
        SoundPresets presets (processor.apvts, prefs, [] {});
        auto setPlain = [&processor] (const char* id, float value)
        {
            subforge::files::setParameterValue (*processor.apvts.getParameter (id), value);
        };

        int changedOffSquare = 0, changedOnSquare = 0;
        for (int i = 0; i < 200; ++i)
        {
            setPlain ("wave", 3.0f); // sine; mutate flips a wave only now and then
            setPlain ("squareWidth", 0.2f);
            presets.mutate();
            if ((int) plainValue (processor, "wave") != 1 && ! juce::approximatelyEqual (plainValue (processor, "squareWidth"), 0.2f))
                ++changedOffSquare;

            setPlain ("wave", 1.0f);
            setPlain ("squareWidth", 0.2f);
            presets.mutate();
            if ((int) plainValue (processor, "wave") == 1 && ! juce::approximatelyEqual (plainValue (processor, "squareWidth"), 0.2f))
                ++changedOnSquare;
        }
        CHECK (changedOffSquare == 0);
        CHECK (changedOnSquare > 100);
    }

    // a name of only characters a file name can't hold is not saved as a hidden, nameless file
    {
        SubForgeProcessor processor;
        SoundPresets presets (processor.apvts, prefs, [] {});
        presets.setFolder (root.getChildFile ("Presets"));
        CHECK (! presets.save ("///"));
        CHECK (! root.getChildFile ("Presets").getChildFile (SoundPresets::kExtension).exists());

        PatternPresets patterns (processor.apvts, processor.getPatternBank(), [root] { return root.getChildFile ("Patterns"); },
                                 [] {});
        CHECK (! patterns.save ("???"));
        CHECK (! root.getChildFile ("Patterns").getChildFile (PatternPresets::kExtension).exists());
    }

    // a root note typed in the host with an absurd octave lands on the nearest end of the range
    {
        SubForgeProcessor processor;
        auto* rootNote = processor.apvts.getParameter ("seqRoot");
        CHECK (juce::approximatelyEqual (rootNote->getValueForText ("C9999999999"), 1.0f));
        CHECK (juce::approximatelyEqual (rootNote->getValueForText ("C-9999999999"), 0.0f));
        CHECK (juce::approximatelyEqual (rootNote->getValueForText ("C3"), rootNote->convertTo0to1 (60.0f)));
    }

   #if JUCE_LINUX
    // a folder name with "$(...)" is opened, not run as a command. A stand-in xdg-open
    // records the folder it was given; PATH holds only it, so no real program can start.
    {
        const auto bin = root.getChildFile ("bin");
        const auto opened = root.getChildFile ("opened"), injected = root.getChildFile ("injected");
        CHECK (bin.createDirectory());
        const auto opener = bin.getChildFile ("xdg-open");
        CHECK (opener.replaceWithText ("#!/bin/sh\nprintf '%s' \"$1\" > \"" + opened.getFullPathName() + ".tmp\" && /bin/mv \""
                                           + opened.getFullPathName() + ".tmp\" \"" + opened.getFullPathName() + "\"\n",
                                       false, false, "\n"));
        CHECK (opener.setExecutePermission (true));
        setenv ("PATH", bin.getFullPathName().toRawUTF8(), 1);
        root.setAsCurrentWorkingDirectory();

        const auto folder = root.getChildFile ("Acid Pack $(: >injected)");
        CHECK (folder.createDirectory());
        CHECK (subforge::files::openFolder (folder));
        for (int waited = 0; waited < 5000 && ! opened.existsAsFile(); waited += 10)
            juce::Thread::sleep (10);
        CHECK (opened.loadFileAsString() == folder.getFullPathName());
        CHECK (! injected.exists());
    }
   #endif

    root.deleteRecursively();
    return finish ("loading");
}
