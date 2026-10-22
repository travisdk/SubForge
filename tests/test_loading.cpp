// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Damaged or hostile projects, preset and pattern files, loaded through the real
// processor (this test links JUCE). Files go to a temporary folder, never the user's.
#include "PluginProcessor.h"
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

    root.deleteRecursively();
    return finish ("loading");
}
