// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <locale>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// Acid-style step sequencer: a 16-step pattern (gate, note, accent, slide)
// played against the host timeline. JUCE-free so it can be tested on its own.
namespace subforge
{

constexpr int kMaxSteps = 16;
constexpr int kMinStepNote = -12, kMaxStepNote = 24; // semitones from the root

// Pattern slots A-H. While the sequencer runs, MIDI notes 0-7 (C-2 to G-2 in Bitwig, C0 to G0 in FL Studio) choose a
// slot instead of transposing; a root note that low would sit below 13 Hz.
constexpr int kNumPatternSlots = 8;
constexpr int kFirstSlotNote = 0;
inline bool isSlotNote (int note) { return note >= kFirstSlotNote && note < kFirstSlotNote + kNumPatternSlots; }

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

// How a pattern plays: its length, the length of each step, swing and gate. Each
// slot has its own, as a pattern file does.
struct Groove
{
    int length = kMaxSteps;
    int rate = 1;        // index into kSeqRateBeats, 1/16
    float swing = 0.5f;  // 0.5 straight .. 0.75 heavy shuffle; delays every second step
    float gate = 0.5f;   // fraction of the step a non-sliding note is held

    double stepBeats() const { return kSeqRateBeats[std::clamp (rate, 0, kNumSeqRates - 1)]; }

    // "length 16 rate 1/16 swing 0.5 gate 0.5", stored with the project. Numbers keep
    // full float precision and always use a decimal point, whatever the host's locale.
    std::string toString() const
    {
        std::ostringstream out;
        out.imbue (std::locale::classic());
        out.precision (9);
        out << "length " << length << " rate " << kSeqRateNames[std::clamp (rate, 0, kNumSeqRates - 1)]
            << " swing " << swing << " gate " << gate;
        return out.str();
    }

    bool fromString (const std::string& text)
    {
        std::istringstream in (text);
        in.imbue (std::locale::classic());
        Groove parsed;
        int found = 0;
        for (std::string key, value; in >> key >> value;)
        {
            std::istringstream number (value);
            number.imbue (std::locale::classic());
            if (key == "length" && number >> parsed.length)
                found |= 1;
            else if (key == "rate")
            {
                for (int i = 0; i < kNumSeqRates; ++i)
                    if (value == kSeqRateNames[i])
                    {
                        parsed.rate = i;
                        found |= 2;
                    }
            }
            else if (key == "swing" && number >> parsed.swing)
                found |= 4;
            else if (key == "gate" && number >> parsed.gate)
                found |= 8;
        }
        if (found != 15)
            return false;
        *this = parsed;
        return true;
    }
};

// Steps packed into atomics so the editor can change them while the audio thread
// plays. The groove fields are atomics of their own for the same reason.
class Pattern
{
public:
    Pattern() { setDefault(); }

    Step get (int index) const { return unpack (steps[(size_t) index].load (std::memory_order_relaxed)); }
    void set (int index, const Step& step) { steps[(size_t) index].store (pack (step), std::memory_order_relaxed); }

    Groove groove() const { return { grooveLength.load(), grooveRate.load(), grooveSwing.load(), grooveGate.load() }; }
    void setGroove (const Groove& g)
    {
        setLength (g.length);
        setRate (g.rate);
        setSwing (g.swing);
        setGate (g.gate);
    }
    void setLength (int value) { grooveLength = std::clamp (value, 1, kMaxSteps); }
    void setRate (int value) { grooveRate = std::clamp (value, 0, kNumSeqRates - 1); }
    // a value that is not a number (from a damaged file) gives the default, as clamp would keep it
    void setSwing (float value) { grooveSwing = std::isnan (value) ? Groove {}.swing : std::clamp (value, 0.5f, 0.75f); }
    void setGate (float value) { grooveGate = std::isnan (value) ? Groove {}.gate : std::clamp (value, 0.05f, 1.0f); }

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

    void copyFrom (const Pattern& other)
    {
        for (int i = 0; i < kMaxSteps; ++i)
            set (i, other.get (i));
        setGroove (other.groove());
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
        std::istringstream in (text);
        for (auto& step : parsed)
        {
            int gate = 0, note = 0, accent = 0, slide = 0;
            char c1 = 0, c2 = 0, c3 = 0;
            if (! (in >> gate >> c1 >> note >> c2 >> accent >> c3 >> slide) || c1 != ':' || c2 != ':' || c3 != ':')
                return false;
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
    std::atomic<int> grooveLength { kMaxSteps }, grooveRate { 1 };
    std::atomic<float> grooveSwing { 0.5f }, grooveGate { 0.5f };
};

// The pattern slots and which one plays. A request for another slot (from the
// Pattern parameter or a slot note) waits until the playing pattern starts over,
// so the switch lands on the loop; the sequencer applies it at its step 0, or
// straight away while it is stopped or after the host jumped. Requests may come
// from any thread.
class PatternBank
{
public:
    Pattern& slot (int index) { return slots[(size_t) std::clamp (index, 0, kNumPatternSlots - 1)]; }
    const Pattern& slot (int index) const { return slots[(size_t) std::clamp (index, 0, kNumPatternSlots - 1)]; }
    Pattern& activePattern() { return slot (active()); }
    const Pattern& activePattern() const { return slot (active()); }

    int active() const { return activeSlot.load(); }
    int pending() const { return pendingSlot.load(); } // -1: none

    void request (int index) { pendingSlot.store (std::clamp (index, 0, kNumPatternSlots - 1)); }
    bool applyPending() // true if another slot plays now
    {
        const int next = pendingSlot.exchange (-1);
        return next >= 0 && activeSlot.exchange (next) != next;
    }
    void select (int index) // at once, e.g. when a project is loaded
    {
        pendingSlot.store (-1);
        activeSlot.store (std::clamp (index, 0, kNumPatternSlots - 1));
    }

private:
    std::array<Pattern, kNumPatternSlots> slots;
    std::atomic<int> activeSlot { 0 }, pendingSlot { -1 };
};

// The grooves of a loaded project. Until 1.2.0 all slots shared one groove, kept
// only in the sequencer parameters, so a slot without a saved groove (empty text)
// gets that one and an old project plays as it did.
inline void restoreGrooves (PatternBank& bank, const std::array<std::string, kNumPatternSlots>& saved, const Groove& shared)
{
    for (int i = 0; i < kNumPatternSlots; ++i)
    {
        Groove groove = shared;
        if (! saved[(size_t) i].empty() && ! groove.fromString (saved[(size_t) i]))
            groove = shared;
        bank.slot (i).setGroove (groove);
    }
}

struct SequencerSettings
{
    bool enabled = false;
    int rootNote = 36;
};

// Start of step k in beats from the timeline's zero; swing delays every second step.
inline double stepStartBeats (int64_t k, const Groove& groove)
{
    const double beats = groove.stepBeats();
    const auto pairStart = (double) (k - (k & 1)) * beats;
    return (k & 1) ? pairStart + 2.0 * beats * groove.swing : pairStart;
}

// Turns the playing slot into note on/off events for one audio block, with the
// groove of that slot. Steps are counted from an anchor on the host timeline:
// beat 0 after a start or a jump, so loops and jumps land on the right step, and
// the start of the new pattern after a switch, so it begins on its first step
// whatever its length and rate. Swing follows the step grid of the timeline.
class SequencerEngine
{
public:
    void reset()
    {
        soundingNote = -1;
        pendingOffPpq = -1.0;
        lastPpqEnd = -1.0;
        anchorToTimeline();
        currentStep.store (-1);
    }

    int getCurrentStep() const { return currentStep.load(); }

    // emit (int sampleOffset, bool noteOn, int note, float velocity)
    template <typename Emit>
    void process (bool playing, double ppqStart, double bpm, double sampleRate, int numSamples,
                  const SequencerSettings& settings, PatternBank& bank, Emit&& emit)
    {
        // Silent before beat 0 (a host's pre-roll), so the pattern starts on its first
        // step at bar 1, and note times are never negative (-1 marks a slide).
        const double ppqPerSample = bpm / 60.0 / sampleRate;
        const double ppqEnd = ppqStart + numSamples * ppqPerSample;
        if (! settings.enabled || ! playing || bpm <= 0.0 || numSamples <= 0 || ppqEnd <= 0.0)
        {
            if (soundingNote >= 0)
                emit (0, false, soundingNote, 0.0f);
            reset();
            bank.applyPending();
            return;
        }

        // the host jumped or looped: stop what is sounding and pick up from here,
        // in the slot asked for at the new position
        if (lastPpqEnd < 0.0)
            bank.applyPending();
        else if (std::abs (ppqStart - lastPpqEnd) > ppqPerSample * 4.0)
        {
            if (soundingNote >= 0)
                emit (0, false, soundingNote, 0.0f);
            soundingNote = -1;
            pendingOffPpq = -1.0;
            anchorToTimeline();
            bank.applyPending();
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

        // A switch in this block starts the new pattern from where the old one was;
        // its later steps can't come before that.
        int64_t firstSample = 0;
        for (bool switched = true; switched;)
        {
            switched = false;
            const auto groove = bank.activePattern().groove();
            const double beats = groove.stepBeats();
            const int length = std::clamp (groove.length, 1, kMaxSteps);
            const auto first = std::max (firstStep, (int64_t) std::floor ((ppqStart - anchorPpq) / beats) - 1);
            const auto last = (int64_t) std::ceil ((ppqEnd - anchorPpq) / beats) + 1;

            for (int64_t j = first; j <= last; ++j)
            {
                const double start = stepStart (j, groove);
                const auto sample = sampleFor (start);
                if (start < 0.0 || sample < firstSample || sample >= numSamples)
                    continue;

                flushNoteOffUpTo (sample);
                const int index = (int) (((j % length) + length) % length);
                if (index == 0 && bank.applyPending())
                {
                    // the new pattern starts here on its first step, on the old one's grid
                    const auto next = bank.activePattern().groove();
                    anchorAt (anchorPpq + (double) j * beats, next);
                    triggerStep (0, start, std::max (start, stepStart (1, next)), (int) sample, next, settings,
                                 bank.activePattern(), emit);
                    firstStep = 1;
                    firstSample = sample;
                    switched = true;
                    break;
                }
                triggerStep (index, start, stepStart (j + 1, groove), (int) sample, groove, settings, bank.activePattern(), emit);
            }
        }

        flushNoteOffUpTo (numSamples - 1);
    }

private:
    // Step j of the playing pattern. With anchorPhase 1 its step 0 sits on an odd
    // step of the timeline's grid, so swing delays the even steps.
    double stepStart (int64_t j, const Groove& groove) const
    {
        return anchorPpq + stepStartBeats (j + anchorPhase, groove) - anchorPhase * groove.stepBeats();
    }

    void anchorToTimeline()
    {
        anchorPpq = 0.0;
        anchorPhase = 0;
        firstStep = std::numeric_limits<int64_t>::min();
    }

    void anchorAt (double ppq, const Groove& groove)
    {
        anchorPpq = ppq;
        const double gridSteps = ppq / groove.stepBeats();
        const auto nearest = std::llround (gridSteps);
        anchorPhase = std::abs (gridSteps - (double) nearest) < 1.0e-6 ? (int) (nearest & 1) : 0;
    }

    template <typename Emit>
    void triggerStep (int index, double start, double nextStart, int sample, const Groove& groove,
                      const SequencerSettings& settings, const Pattern& pattern, Emit& emit)
    {
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
        const double duration = nextStart - start;
        pendingOffPpq = step.slide ? -1.0 : start + duration * std::clamp ((double) groove.gate, 0.05, 1.0);
    }

    int soundingNote = -1;
    double pendingOffPpq = -1.0;
    double lastPpqEnd = -1.0;
    double anchorPpq = 0.0;  // where step 0 of the playing pattern falls
    int anchorPhase = 0;     // 1: step 0 falls on an odd step of the timeline's grid
    int64_t firstStep = std::numeric_limits<int64_t>::min(); // 1 after a switch: step 0 has played
    std::atomic<int> currentStep { -1 };
};

//==============================================================================
// One pass of a pattern as notes for a MIDI clip, played as the sequencer plays it:
// the same velocities, gate and swing, and a slide holds the note into the next
// step. A slide to a different note overlaps it slightly, so a mono synth glides;
// a slide to the same note becomes one long note.
struct ClipNote
{
    double start, end; // beats from the start of the clip
    int note;
    int velocity;      // 1..127
};

struct PatternClip
{
    std::vector<ClipNote> notes;
    double lengthBeats = 0.0;
};

constexpr double kSlideOverlapBeats = 1.0 / 64.0;

inline int midiVelocity (float velocity) { return std::clamp ((int) std::lround (velocity * 127.0f), 1, 127); }

inline PatternClip patternToClip (const Pattern& pattern, int rootNote)
{
    PatternClip clip;
    const auto groove = pattern.groove();
    const int length = std::clamp (groove.length, 1, kMaxSteps);
    clip.lengthBeats = length * groove.stepBeats();

    bool tied = false; // the last note slides into the next step
    for (int i = 0; i < length; ++i)
    {
        const auto step = pattern.get (i);
        const double start = stepStartBeats (i, groove);
        const double duration = stepStartBeats (i + 1, groove) - start;
        const double end = step.slide ? start + duration
                                      : start + duration * std::clamp ((double) groove.gate, 0.05, 1.0);

        if (! step.gate)
        {
            tied = false; // a rest ends a slide at its start, where the held note already ends
            continue;
        }

        const int note = std::clamp (rootNote + step.note, 0, 127);
        if (tied && clip.notes.back().note == note)
            clip.notes.back().end = end;
        else
        {
            if (tied)
                clip.notes.back().end = start + std::min (kSlideOverlapBeats, duration * 0.5);
            clip.notes.push_back ({ start, end, note, midiVelocity (step.accent ? kSeqAccentVelocity : kSeqVelocity) });
        }
        tied = step.slide;
    }

    for (auto& n : clip.notes)
        n.end = std::min (n.end, clip.lengthBeats);
    return clip;
}

} // namespace subforge
