// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Renders the audio demo: factory sound presets, each played by a factory pattern
// from the sequencer for a few bars, joined on the bar lines.
//   SubForge_RenderDemo <output.wav> <timecodes.txt>
// Writes an unmastered 32-bit float WAV at 48 kHz; loudness is set afterwards
// with ffmpeg. Each section runs in its own plug-in instance against a
// shared timeline, so the sequencer stays on the beat across the joins.
//
// Null test before a release, against a render of the previous release:
//   SubForge_RenderDemo --compare <reference.wav> <new.wav>
// Subtracts the two renders and prints the largest difference per section.
// The synth is deterministic, but compilers may round differently, so the
// renders need not be bit-identical; differences below kInaudibleDb count as
// the same. Exits with 1 if any section differs.

#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr double kBpm = 124.0;
constexpr int kBlockSize = 512;
constexpr int kBarsPerSection = 7;
constexpr int kRootNote = 36;          // C2
constexpr double kTailSeconds = 0.35;  // release after the last note of a section, faded out
constexpr double kEndSeconds = 1.0;    // room after the last section for its tail
constexpr double kInaudibleDb = -90.0; // a larger difference between two renders is a real change

struct Section
{
    const char* preset;
    const char* pattern;
    const char* category;
    float gainDb;  // evens out the sections' loudness
};

const Section kSections[] = {
    { "Acid Classic",         "Chicago Jack",      "Acid",       0.0f },
    { "Funk Pluck",           "Funky Slap",        "Pluck",      1.5f },
    { "Dub Bass",             "Deep House Groove", "Sub / Dub",  5.0f },
    { "Detune Growl",         "Body Drive",        "Detune",     1.0f },
    { "Classic Ladder",       "Electro Funk",      "Ladder",     1.5f },
    { "Round Bass",           "Disco Octaves",     "Warm",       0.8f },
};

struct Transport : juce::AudioPlayHead
{
    double ppq = 0.0;
    bool playing = true;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm (kBpm);
        info.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        info.setPpqPosition (ppq);
        info.setIsPlaying (playing);
        return info;
    }
};

template <typename Entries>
int findEntry (const Entries& entries, const juce::String& name)
{
    for (int i = 0; i < entries.numEntries(); ++i)
        if (entries.entryName (i) == name)
            return i;
    return -1;
}

int64_t sectionLength() { return (int64_t) std::llround (kBarsPerSection * 4 * kSampleRate * 60.0 / kBpm); }

juce::String timecode (double seconds)
{
    const int total = juce::roundToInt (std::floor (seconds));
    return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
}

std::unique_ptr<juce::AudioBuffer<float>> readWav (const juce::String& path)
{
    juce::WavAudioFormat wav;
    const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (path);
    std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (new juce::FileInputStream (file), true));
    if (reader == nullptr || juce::roundToInt (reader->sampleRate) != (int) kSampleRate || reader->numChannels != 2)
    {
        std::cerr << "not a 48 kHz stereo WAV from this tool: " << file.getFullPathName() << "\n";
        return nullptr;
    }
    auto buffer = std::make_unique<juce::AudioBuffer<float>> (2, (int) reader->lengthInSamples);
    reader->read (buffer.get(), 0, (int) reader->lengthInSamples, 0, true, true);
    return buffer;
}

juce::String decibels (float gain, const char* whenZero)
{
    return gain <= 0.0f ? juce::String (whenZero) : juce::String (juce::Decibels::gainToDecibels (gain, -1000.0f), 1) + " dB";
}

// Peak of the difference per section, in dBFS; the last section includes the tail at the end.
int compareDemos (const juce::String& referencePath, const juce::String& newPath)
{
    const auto reference = readWav (referencePath);
    const auto rendered = readWav (newPath);
    if (reference == nullptr || rendered == nullptr)
        return 2;
    if (reference->getNumSamples() != rendered->getNumSamples())
    {
        std::cout << "DIFFERENT LENGTH: " << reference->getNumSamples() << " vs " << rendered->getNumSamples()
                  << " samples - the demo itself changed, so render a new reference\n";
        return 1;
    }

    const auto numSections = (int64_t) std::size (kSections);
    bool allSame = true;
    for (int64_t s = 0; s < numSections; ++s)
    {
        const int start = (int) (s * sectionLength());
        const int end = s == numSections - 1 ? reference->getNumSamples() : (int) ((s + 1) * sectionLength());
        float peakDifference = 0.0f, peakLevel = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = start; i < end; ++i)
            {
                const float a = reference->getSample (ch, i), b = rendered->getSample (ch, i);
                peakDifference = std::max (peakDifference, std::abs (a - b));
                peakLevel = std::max (peakLevel, std::abs (a));
            }

        const bool same = juce::Decibels::gainToDecibels (peakDifference, -1000.0f) < kInaudibleDb;
        allSame = allSame && same;
        std::cout << (same ? "OK       " : "CHANGED  ") << timecode ((double) start / kSampleRate) << "  "
                  << juce::String (kSections[s].preset).paddedRight (' ', 16) << " difference "
                  << decibels (peakDifference, "none").paddedLeft (' ', 9) << "  (level " << decibels (peakLevel, "silent") << ")\n";
    }

    std::cout << (allSame ? "All sections sound the same.\n"
                          : "Some sections changed. Fine if intended and in CHANGELOG.md; otherwise find out why.\n");
    return allSame ? 0 : 1;
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc == 4 && juce::String (argv[1]) == "--compare")
        return compareDemos (argv[2], argv[3]);

    if (argc < 3)
    {
        std::cerr << "usage: SubForge_RenderDemo <output.wav> <timecodes.txt>\n"
                     "       SubForge_RenderDemo --compare <reference.wav> <new.wav>\n";
        return 1;
    }

    const juce::ScopedJuceInitialiser_GUI gui;
    const double samplesPerBeat = kSampleRate * 60.0 / kBpm;
    const auto sectionSamples = sectionLength();
    const auto tailSamples = (int64_t) (kTailSeconds * kSampleRate);
    const auto numSections = (int64_t) std::size (kSections);
    const auto totalSamples = numSections * sectionSamples + (int64_t) (kEndSeconds * kSampleRate);

    juce::AudioBuffer<float> mix (2, (int) totalSamples);
    mix.clear();
    juce::String timecodes;
    timecodes << "SubForge demo – " << kBpm << " BPM, " << kBarsPerSection << " bars per sound, root C2\n\n";

    for (int64_t s = 0; s < numSections; ++s)
    {
        const auto& section = kSections[s];
        const std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        auto& processor = dynamic_cast<SubForgeProcessor&> (*plugin);

        const int preset = findEntry (processor.getSoundPresets(), section.preset);
        const int pattern = findEntry (processor.getPatternPresets(), section.pattern);
        if (preset < 0 || pattern < 0)
        {
            std::cerr << "unknown preset or pattern: " << section.preset << " / " << section.pattern << "\n";
            return 1;
        }
        processor.getSoundPresets().load (preset);
        processor.getPatternPresets().load (pattern);
        auto set = [&] (const char* id, float value)
        {
            auto* param = processor.apvts.getParameter (id);
            param->setValueNotifyingHost (param->convertTo0to1 (value));
        };
        set ("seqOn", 1.0f);
        set ("seqRoot", (float) kRootNote);

        Transport transport;
        processor.setPlayHead (&transport);
        processor.setPlayConfigDetails (0, 2, kSampleRate, kBlockSize);
        processor.prepareToPlay (kSampleRate, kBlockSize);
        const int latency = processor.getLatencySamples();

        // the section's own bars, then the release of its last note with the transport stopped
        const int64_t start = s * sectionSamples;
        const int64_t length = sectionSamples + tailSamples + latency;
        const float gain = juce::Decibels::decibelsToGain (section.gainDb);
        juce::AudioBuffer<float> block (2, kBlockSize);
        juce::MidiBuffer midi;

        for (int64_t pos = 0; pos < length; pos += kBlockSize)
        {
            const int n = (int) std::min<int64_t> (kBlockSize, length - pos);
            transport.playing = pos < sectionSamples;
            transport.ppq = (double) (start + pos) / samplesPerBeat;
            block.setSize (2, n, false, false, true);
            midi.clear();
            processor.processBlock (block, midi);

            for (int i = 0; i < n; ++i)
            {
                const int64_t out = pos + i - latency; // position within the section
                if (out < 0 || start + out >= totalSamples)
                    continue;
                float g = gain;
                if (out >= sectionSamples) // raised-cosine fade over the tail
                    g *= 0.5f * (1.0f + std::cos (juce::MathConstants<float>::pi
                                                  * (float) (out - sectionSamples) / (float) tailSamples));
                for (int ch = 0; ch < 2; ++ch)
                    mix.addSample (ch, (int) (start + out), block.getSample (ch, i) * g);
            }
        }
        processor.releaseResources();

        timecodes << timecode ((double) start / kSampleRate) << "  " << section.preset << "  (" << section.category
                  << ")  –  pattern: " << section.pattern << "\n";
        std::cout << timecode ((double) start / kSampleRate) << "  " << section.preset << " / " << section.pattern
                  << "\n";
    }

    const juce::File wavFile = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    wavFile.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (wavFile);
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions()
                                                   .withSampleRate (kSampleRate)
                                                   .withNumChannels (2)
                                                   .withBitsPerSample (32)
                                                   .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
    if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (mix, 0, mix.getNumSamples()))
    {
        std::cerr << "could not write " << wavFile.getFullPathName() << "\n";
        return 1;
    }
    writer.reset();

    const juce::File textFile = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
    if (! textFile.replaceWithText (timecodes))
    {
        std::cerr << "could not write " << textFile.getFullPathName() << "\n";
        return 1;
    }
    std::cout << "wrote " << wavFile.getFullPathName() << " (" << (double) totalSamples / kSampleRate << " s)\n";
    return 0;
}
