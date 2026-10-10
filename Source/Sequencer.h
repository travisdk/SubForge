#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <random>
#include <string>

// Acid-style step sequencer: a 16-step pattern (gate, note, accent, slide)
// played against the host timeline. JUCE-free so it can be tested on its own.
namespace subforge
{

constexpr int kMaxSteps = 16;
constexpr int kMinStepNote = -12, kMaxStepNote = 24; // semitones from the root

// Sequencer rate choices, in quarter-note beats per step.
inline constexpr float kSeqRateBeats[] = { 0.5f, 0.25f, 0.125f, 1.0f / 3.0f, 1.0f / 6.0f };
inline constexpr const char* kSeqRateNames[] = { "1/8", "1/16", "1/32", "1/8T", "1/16T" };
constexpr int kNumSeqRates = (int) std::size (kSeqRateBeats);

constexpr float kSeqVelocity = 0.7f;       // below the accent threshold
constexpr float kSeqAccentVelocity = 1.0f;

struct Step
{
    bool gate = true;
    int note = 0;
    bool accent = false;
    bool slide = false;

    bool operator== (const Step& other) const
    {
        return gate == other.gate && note == other.note && accent == other.accent && slide == other.slide;
    }
};

// Steps packed into atomics so the editor can change them while the audio thread plays.
class Pattern
{
public:
    Pattern() { setDefault(); }

    Step get (int index) const { return unpack (steps[(size_t) index].load (std::memory_order_relaxed)); }
    void set (int index, const Step& step) { steps[(size_t) index].store (pack (step), std::memory_order_relaxed); }

    // A short acid line so the sequencer does something sensible straight away.
    void setDefault()
    {
        const Step line[kMaxSteps] = {
            { true, 0, true, false },  { true, 0, false, false },  { true, 12, false, true }, { true, 0, false, false },
            { true, 3, true, false },  { false, 0, false, false }, { true, 0, false, true },  { true, 7, false, false },
            { true, 0, true, false },  { true, 12, false, false }, { true, 10, false, true }, { true, 12, true, false },
            { true, 0, false, false }, { true, 3, false, true },   { true, 5, false, false }, { true, 0, true, false },
        };
        for (int i = 0; i < kMaxSteps; ++i)
            set (i, line[i]);
    }

    void clear()
    {
        for (int i = 0; i < kMaxSteps; ++i)
            set (i, { true, 0, false, false });
    }

    // "gate:note:accent:slide" per step, separated by spaces, e.g. "1:0:1:0 1:12:0:1 ..."
    std::string toString() const
    {
        std::string text;
        for (int i = 0; i < kMaxSteps; ++i)
        {
            const auto s = get (i);
            text += (i > 0 ? " " : "") + std::to_string (s.gate) + ":" + std::to_string (s.note) + ":"
                    + std::to_string (s.accent) + ":" + std::to_string (s.slide);
        }
        return text;
    }

    bool fromString (const std::string& text)
    {
        Step parsed[kMaxSteps];
        size_t pos = 0;
        for (auto& step : parsed)
        {
            int gate, note, accent, slide, consumed = 0;
            if (std::sscanf (text.c_str() + pos, " %d:%d:%d:%d%n", &gate, &note, &accent, &slide, &consumed) != 4)
                return false;
            pos += (size_t) consumed;
            step = { gate != 0, std::clamp (note, kMinStepNote, kMaxStepNote), accent != 0, slide != 0 };
        }
        for (int i = 0; i < kMaxSteps; ++i)
            set (i, parsed[i]);
        return true;
    }

    // Random acid line: rooted, scale-bound, octave jumps, some accents and slides.
    void randomize (std::mt19937& rng)
    {
        static constexpr int scales[3][8] = {
            { 0, 3, 5, 7, 10, -1, -1, -1 }, // minor pentatonic
            { 0, 2, 3, 5, 7, 8, 10, -1 },   // natural minor
            { 0, 1, 3, 5, 7, 8, 10, -1 },   // phrygian
        };
        std::uniform_real_distribution<float> uniform (0.0f, 1.0f);
        auto chance = [&] (float p) { return uniform (rng) < p; };
        const auto& scale = scales[std::uniform_int_distribution<int> (0, 2) (rng)];
        const int scaleSize = (int) (std::find (std::begin (scale), std::end (scale), -1) - std::begin (scale));

        for (int i = 0; i < kMaxSteps; ++i)
        {
            Step s;
            s.gate = i == 0 || chance (0.8f);
            s.note = (i == 0 || chance (0.45f)) ? 0 : scale[std::uniform_int_distribution<int> (1, scaleSize - 1) (rng)];
            if (chance (0.2f))
                s.note += 12;
            else if (chance (0.05f))
                s.note -= 12;
            s.accent = i == 0 || chance (0.25f);
            s.slide = i < kMaxSteps - 1 && chance (0.2f);
            set (i, s);
        }
    }

private:
    static uint32_t pack (const Step& s)
    {
        return (s.gate ? 1u : 0u) | (s.accent ? 2u : 0u) | (s.slide ? 4u : 0u) | ((uint32_t) (s.note + 128) << 8);
    }
    static Step unpack (uint32_t v)
    {
        return { (v & 1u) != 0, (int) ((v >> 8) & 0xffu) - 128, (v & 2u) != 0, (v & 4u) != 0 };
    }

    std::array<std::atomic<uint32_t>, kMaxSteps> steps;
};

struct SequencerSettings
{
    bool enabled = false;
    float stepBeats = 0.25f;
    int length = 16;
    float gate = 0.5f;   // fraction of the step a non-sliding note is held
    float swing = 0.5f;  // 0.5 straight .. 0.75 heavy shuffle; delays every second step
    int rootNote = 36;
};

// Turns the pattern into note on/off events for one audio block. Steps are
// locked to the host timeline (step k starts at beat k * stepBeats), so loops
// and jumps land on the right step.
class SequencerEngine
{
public:
    void reset()
    {
        soundingNote = -1;
        pendingOffPpq = -1.0;
        lastPpqEnd = -1.0;
        currentStep.store (-1);
    }

    int getCurrentStep() const { return currentStep.load(); }

    // emit (int sampleOffset, bool noteOn, int note, float velocity)
    template <typename Emit>
    void process (bool playing, double ppqStart, double bpm, double sampleRate, int numSamples,
                  const SequencerSettings& settings, const Pattern& pattern, Emit&& emit)
    {
        if (! settings.enabled || ! playing || bpm <= 0.0 || numSamples <= 0)
        {
            if (soundingNote >= 0)
                emit (0, false, soundingNote, 0.0f);
            reset();
            return;
        }

        const double ppqPerSample = bpm / 60.0 / sampleRate;
        const double ppqEnd = ppqStart + numSamples * ppqPerSample;

        // the host jumped or looped: stop what is sounding and pick up from here
        if (lastPpqEnd >= 0.0 && std::abs (ppqStart - lastPpqEnd) > ppqPerSample * 4.0)
        {
            if (soundingNote >= 0)
                emit (0, false, soundingNote, 0.0f);
            soundingNote = -1;
            pendingOffPpq = -1.0;
        }
        lastPpqEnd = ppqEnd;

        // An event happens on the first sample at or after its beat position. Deciding
        // block membership by that sample keeps timing independent of the block size.
        auto sampleFor = [&] (double ppq) { return (int64_t) std::ceil ((ppq - ppqStart) / ppqPerSample - 1.0e-6); };
        auto flushNoteOffUpTo = [&] (int64_t sample) // note-offs at or before sample
        {
            if (pendingOffPpq >= 0.0 && soundingNote >= 0)
            {
                const auto offSample = sampleFor (pendingOffPpq);
                if (offSample <= sample)
                {
                    emit ((int) std::clamp<int64_t> (offSample, 0, numSamples - 1), false, soundingNote, 0.0f);
                    soundingNote = -1;
                    pendingOffPpq = -1.0;
                }
            }
        };

        const double beats = settings.stepBeats;
        const auto first = (int64_t) std::floor (ppqStart / beats) - 1;
        const auto last = (int64_t) std::ceil (ppqEnd / beats) + 1;

        for (int64_t k = first; k <= last; ++k)
        {
            const double start = stepStart (k, settings);
            const auto sample = sampleFor (start);
            if (sample < 0 || sample >= numSamples)
                continue;

            flushNoteOffUpTo (sample);
            triggerStep (k, start, (int) sample, settings, pattern, emit);
        }

        flushNoteOffUpTo (numSamples - 1);
    }

private:
    static double stepStart (int64_t k, const SequencerSettings& settings)
    {
        const double beats = settings.stepBeats;
        const auto pairStart = (double) (k - (k & 1)) * beats;
        return (k & 1) ? pairStart + 2.0 * beats * settings.swing : pairStart;
    }

    template <typename Emit>
    void triggerStep (int64_t k, double start, int sample, const SequencerSettings& settings, const Pattern& pattern, Emit& emit)
    {
        const int length = std::clamp (settings.length, 1, kMaxSteps);
        const int index = (int) (((k % length) + length) % length);
        currentStep.store (index);

        const auto step = pattern.get (index);
        const bool tiedIn = soundingNote >= 0 && pendingOffPpq < 0.0; // previous step slid into this one

        if (! step.gate)
        {
            if (soundingNote >= 0)
                emit (sample, false, soundingNote, 0.0f);
            soundingNote = -1;
            pendingOffPpq = -1.0;
            return;
        }

        const int note = std::clamp (settings.rootNote + step.note, 0, 127);
        const float velocity = step.accent ? kSeqAccentVelocity : kSeqVelocity;

        if (tiedIn)
        {
            // legato: new note first so the voice glides instead of retriggering
            if (note != soundingNote)
            {
                emit (sample, true, note, velocity);
                emit (sample, false, soundingNote, 0.0f);
            }
        }
        else
        {
            if (soundingNote >= 0)
                emit (sample, false, soundingNote, 0.0f);
            emit (sample, true, note, velocity);
        }

        soundingNote = note;
        const double duration = stepStart (k + 1, settings) - start;
        pendingOffPpq = step.slide ? -1.0 : start + duration * std::clamp ((double) settings.gate, 0.05, 1.0);
    }

    int soundingNote = -1;
    double pendingOffPpq = -1.0;
    double lastPpqEnd = -1.0;
    std::atomic<int> currentStep { -1 };
};

} // namespace subforge
