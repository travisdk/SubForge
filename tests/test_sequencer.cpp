// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Sequencer engine: timing independent of the host block size, swing,
// no stuck notes after slides, loops or stopping, and pattern slots that
// switch when the pattern starts over (each with its own length and rate, and
// old projects as 1.1.0 played them), and MIDI clips that play like the sequencer.
#include "Sequencer.h"
#include "TestUtils.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using namespace subforge;

struct Event
{
    long sample;
    bool on;
    int note;
};

static std::vector<Event> run (int blockSize, SequencerSettings settings, const Pattern& pattern, double seconds,
                               double loopAtBeat = -1.0, double startBeat = 0.0)
{
    constexpr double sampleRate = 48000.0, bpm = 120.0;
    SequencerEngine engine;
    PatternBank bank;
    bank.slot (0).copyFrom (pattern);
    std::vector<Event> events;
    double ppq = startBeat;
    long t = 0;
    while (t < seconds * sampleRate)
    {
        if (loopAtBeat > 0.0 && ppq >= loopAtBeat)
        {
            ppq = 0.0;
            loopAtBeat = -1.0;
        }
        engine.process (true, ppq, bpm, sampleRate, blockSize, settings, bank,
                        [&] (int s, bool on, int note, float) { events.push_back ({ t + s, on, note }); });
        ppq += blockSize * bpm / 60.0 / sampleRate;
        t += blockSize;
    }
    engine.process (false, 0.0, bpm, sampleRate, 64, settings, bank,
                    [&] (int s, bool on, int note, float) { events.push_back ({ t + s, on, note }); });
    return events;
}

// Slot A plays the root, slot B a fifth above. Asks for B at requestAt samples
// (and, if jumpAt > 0, jumps back to beat 0 there) and returns the note-on times
// and notes.
static std::vector<Event> runSlots (int blockSize, long requestAt, long jumpAt = -1)
{
    constexpr double sampleRate = 48000.0, bpm = 120.0;
    SequencerSettings settings;
    settings.enabled = true;
    SequencerEngine engine;
    PatternBank bank;
    bank.slot (0).clear();
    bank.slot (1).clear();
    for (int i = 0; i < kMaxSteps; ++i)
        bank.slot (1).set (i, { true, 7, false, false });

    std::vector<Event> events;
    double ppq = 0.0;
    bool requested = false;
    for (long t = 0; t < 4 * 96000; t += blockSize)
    {
        if (! requested && t + blockSize > requestAt)
        {
            bank.request (1);
            requested = true;
            if (jumpAt > 0)
                ppq = 0.0;
        }
        engine.process (true, ppq, bpm, sampleRate, blockSize, settings, bank,
                        [&] (int s, bool on, int note, float)
                        {
                            if (on)
                                events.push_back ({ t + s, on, note });
                        });
        ppq += blockSize * bpm / 60.0 / sampleRate;
    }
    return events;
}

// Plays the bank from beat 0 at 120 bpm and 48 kHz, starting in slot A. Each request
// asks for its slot in the block that holds its sample. Returns the note-ons before
// the given number of samples.
struct Request
{
    long at;
    int slot;
};

static std::vector<Event> runBank (PatternBank& bank, int blockSize, long samples, std::vector<Request> requests)
{
    SequencerSettings settings;
    settings.enabled = true;
    SequencerEngine engine;
    bank.select (0);
    std::vector<Event> ons;
    for (long t = 0; t < samples; t += blockSize)
    {
        for (const auto& r : requests)
            if (r.at >= t && r.at < t + blockSize)
                bank.request (r.slot);
        engine.process (true, t / 24000.0, 120.0, 48000.0, blockSize, settings, bank,
                        [&] (int s, bool on, int note, float)
                        {
                            if (on && t + s < samples)
                                ons.push_back ({ t + s, on, note });
                        });
    }
    return ons;
}

static bool sameOns (const std::vector<Event>& played, const std::vector<Event>& expected)
{
    if (played.size() != expected.size())
        return false;
    for (size_t i = 0; i < played.size(); ++i)
        if (std::abs (played[i].sample - expected[i].sample) > 1 || played[i].note != expected[i].note)
            return false;
    return true;
}

static bool sameGroove (const Groove& a, const Groove& b)
{
    return a.toString() == b.toString(); // every field, floats to full precision
}

// Every step plays, without accents or slides, from firstNote upwards.
static void fillScale (Pattern& p, int firstNote, const Groove& groove)
{
    for (int i = 0; i < kMaxSteps; ++i)
        p.set (i, { true, firstNote + i, false, false });
    p.setGroove (groove);
}

// How 1.1.0 played: one groove for all slots, step k of the timeline at
// stepStartBeats (k), a requested slot taking over at the first step 0 it reaches.
static std::vector<Event> playedBy110 (const PatternBank& bank, const Groove& shared, int blockSize, long samples,
                                       const std::vector<Request>& requests)
{
    std::vector<Event> ons;
    int active = 0;
    size_t nextRequest = 0;
    int pending = -1;
    for (int64_t k = 0;; ++k)
    {
        const auto sample = (long) std::ceil (stepStartBeats (k, shared) * 24000.0 - 1.0e-6);
        if (sample >= samples)
            break;
        const long block = sample / blockSize * blockSize;
        for (; nextRequest < requests.size() && requests[nextRequest].at / blockSize * blockSize <= block; ++nextRequest)
            pending = requests[nextRequest].slot;
        const int index = (int) (k % shared.length);
        if (index == 0 && pending >= 0)
        {
            active = pending;
            pending = -1;
        }
        const auto step = bank.slot (active).get (index);
        if (step.gate)
            ons.push_back ({ sample, true, 36 + step.note });
    }
    return ons;
}

static long firstNoteOf (const std::vector<Event>& events, int note)
{
    for (const auto& e : events)
        if (e.note == note)
            return e.sample;
    return -1;
}

static int openNotes (const std::vector<Event>& events)
{
    int open = 0;
    for (const auto& e : events)
        open += e.on ? 1 : -1;
    return open;
}

int main()
{
    Pattern pattern; // default acid line with accents and slides
    SequencerSettings settings;
    settings.enabled = true;

    // identical event times for different block sizes (compare the common span)
    const auto a = run (64, settings, pattern, 4.0), b = run (1024, settings, pattern, 4.0), c = run (441, settings, pattern, 4.0);
    const long horizon = 180000;
    auto prefix = [&] (const std::vector<Event>& events)
    {
        std::vector<Event> out;
        for (const auto& e : events)
            if (e.sample < horizon)
                out.push_back (e);
        return out;
    };
    const auto pa = prefix (a), pb = prefix (b), pc = prefix (c);
    CHECK (pa.size() == pb.size() && pa.size() == pc.size());
    for (size_t i = 0; i < pa.size() && i < pb.size() && i < pc.size(); ++i)
    {
        CHECK (pa[i].sample == pb[i].sample && pa[i].on == pb[i].on && pa[i].note == pb[i].note);
        CHECK (pa[i].sample == pc[i].sample && pa[i].on == pc[i].on && pa[i].note == pc[i].note);
    }

    // no stuck notes at the end, after a loop jump, and at most two notes overlap (slides)
    CHECK (openNotes (a) == 0);
    CHECK (openNotes (run (512, settings, pattern, 3.0, 2.0)) == 0);
    int open = 0, maxOpen = 0;
    for (const auto& e : a)
    {
        open += e.on ? 1 : -1;
        maxOpen = std::max (maxOpen, open);
    }
    CHECK (maxOpen <= 2);

    // swing 0.66 at 1/16 and 120 bpm: odd steps start at 0.66 * 12000 samples
    Pattern straight;
    straight.clear();
    straight.setSwing (0.66f);
    std::vector<long> ons;
    for (const auto& e : run (256, settings, straight, 0.5))
        if (e.on)
            ons.push_back (e.sample);
    CHECK (ons.size() >= 4);
    if (ons.size() >= 4)
    {
        CHECK (ons[0] == 0);
        CHECK (std::abs (ons[1] - 7920) <= 1);
        CHECK (ons[2] == 12000);
        CHECK (std::abs (ons[3] - 19920) <= 1);
    }

    // 16 steps of 1/16 at 120 bpm loop every 96000 samples. A request in the middle of
    // the first loop switches at the start of the second, for any block size.
    for (int blockSize : { 64, 441, 512, 1024 })
    {
        const auto events = runSlots (blockSize, 30000);
        CHECK (firstNoteOf (events, 43) == 96000);
        CHECK (firstNoteOf (events, 36) == 0);
        long lastRoot = -1;
        for (const auto& e : events)
            if (e.note == 36)
                lastRoot = e.sample;
        CHECK (lastRoot < 96000);
    }

    // after a jump the requested slot plays straight away
    {
        const auto events = runSlots (512, 30208, 30208);
        const long jumpBlock = 30208 / 512 * 512;
        CHECK (firstNoteOf (events, 43) == jumpBlock);
    }

    // while stopped a request is applied at once
    {
        SequencerEngine engine;
        PatternBank bank;
        SequencerSettings settings;
        settings.enabled = true;
        bank.request (3);
        CHECK (bank.active() == 0 && bank.pending() == 3);
        engine.process (false, 0.0, 120.0, 48000.0, 512, settings, bank, [] (int, bool, int, float) {});
        CHECK (bank.active() == 3 && bank.pending() == -1);
        bank.request (5);
        bank.select (2);
        CHECK (bank.active() == 2 && bank.pending() == -1);
    }

    // a clip starts each note where the sequencer does, with its note and velocity
    for (const float swing : { 0.5f, 0.62f })
    {
        SequencerSettings clipSettings;
        clipSettings.enabled = true;
        Pattern line; // default line: accents, slides to other notes, a rest
        line.setSwing (swing);
        const auto clip = patternToClip (line, clipSettings.rootNote);
        CHECK (std::abs (clip.lengthBeats - 4.0) < 1e-9);

        SequencerEngine engine;
        PatternBank bank;
        bank.slot (0).copyFrom (line);
        struct On { long sample; int note; int velocity; };
        std::vector<On> played;
        for (long t = 0; t < 96000; t += 480) // one pass: 4 beats at 120 bpm
            engine.process (true, t / 24000.0, 120.0, 48000.0, 480, clipSettings, bank,
                            [&] (int s, bool on, int note, float velocity)
                            {
                                if (on && t + s < 96000)
                                    played.push_back ({ t + s, note, midiVelocity (velocity) });
                            });
        CHECK (played.size() == clip.notes.size());
        for (size_t i = 0; i < played.size() && i < clip.notes.size(); ++i)
        {
            CHECK (std::abs (played[i].sample - clip.notes[i].start * 24000.0) <= 1.0);
            CHECK (played[i].note == clip.notes[i].note);
            CHECK (played[i].velocity == clip.notes[i].velocity);
        }
    }

    // gate, slides and rests
    {
        Pattern p; // 1/16, gate 0.5; root C2 (36)
        p.clear();
        p.set (0, { true, 0, true, false });   // accent, gate 0.5
        p.set (1, { true, 0, false, true });   // slide to another note: overlaps it
        p.set (2, { true, 7, false, true });   // slide to the same note: one long note
        p.set (3, { true, 7, false, false });
        p.set (4, { true, 3, false, true });   // slide into a rest: ends where the rest starts
        p.set (5, { false, 0, false, false });
        p.set (15, { true, 12, false, true }); // a slide on the last step ends with the clip
        const auto clip = patternToClip (p, 36);
        CHECK (clip.notes.size() >= 4);
        if (clip.notes.size() >= 4)
        {
            const auto& n = clip.notes;
            CHECK (n[0].start == 0.0 && std::abs (n[0].end - 0.125) < 1e-9 && n[0].velocity == 127);
            CHECK (n[1].start == 0.25 && std::abs (n[1].end - (0.5 + kSlideOverlapBeats)) < 1e-9 && n[1].velocity == 89);
            CHECK (n[2].start == 0.5 && n[2].note == 43 && std::abs (n[2].end - 0.875) < 1e-9);
            CHECK (n[3].start == 1.0 && n[3].note == 39 && std::abs (n[3].end - 1.25) < 1e-9);
            CHECK (n.back().note == 48 && std::abs (n.back().end - 4.0) < 1e-9);
        }
        p.setLength (7);
        p.setRate (0); // 1/8
        CHECK (std::abs (patternToClip (p, 36).lengthBeats - 3.5) < 1e-9);
    }

    // a clip is steps x rate long, whole bars or not
    {
        struct Case { int steps; int rate; double beats; };
        const Case cases[] = {
            { 16, 1, 4.0 }, // one bar
            { 9, 1, 2.25 }, // nine 1/16
            { 12, 1, 3.0 },
            { 4, 1, 1.0 },  // one beat
            { 8, 0, 4.0 },  // one bar of 1/8
            { 9, 4, 1.5 },  // nine 1/16T
        };
        for (const auto& c : cases)
        {
            Pattern p;
            p.setLength (c.steps);
            p.setRate (c.rate);
            const auto clip = patternToClip (p, 36);
            CHECK (std::abs (clip.lengthBeats - c.beats) < 1e-6);
            for (const auto& n : clip.notes)
                CHECK (n.end <= clip.lengthBeats);
        }
    }

    // Slots with their own length and rate: a switch lands when the playing pattern
    // starts over, and the new one starts on its first step with its own rate.
    // A: 7 steps of 1/16 (6000 samples), B: 5 steps of 1/8 (12000 samples).
    for (int blockSize : { 64, 441, 512, 1024 })
    {
        PatternBank bank;
        fillScale (bank.slot (0), 0, { 7, 1, 0.5f, 0.5f });
        fillScale (bank.slot (1), 12, { 5, 0, 0.5f, 0.5f });
        std::vector<Event> expected;
        for (int i = 0; i < 7; ++i)
            expected.push_back ({ i * 6000L, true, 36 + i });        // A from beat 0
        for (int i = 0; i < 5; ++i)
            expected.push_back ({ 42000 + i * 12000L, true, 48 + i }); // B after one pass of A
        for (int i = 0; i < 7; ++i)
            expected.push_back ({ 102000 + i * 6000L, true, 36 + i }); // A after one pass of B
        CHECK (sameOns (runBank (bank, blockSize, 144000, { { 30000, 1 }, { 50000, 0 } }), expected));
    }

    // from 1/16 to six 1/16T steps (4000 samples) that loop on their own length
    {
        PatternBank bank;
        fillScale (bank.slot (0), 0, {});
        fillScale (bank.slot (1), 12, { 6, 4, 0.5f, 0.5f });
        const auto ons = runBank (bank, 512, 96000 + 12 * 4000, { { 1000, 1 } });
        CHECK (ons.size() == 16 + 12);
        for (int i = 0; i < 12 && 16 + i < (int) ons.size(); ++i)
            CHECK (std::abs (ons[(size_t) (16 + i)].sample - (96000 + i * 4000L)) <= 1 && ons[(size_t) (16 + i)].note == 48 + i % 6);
    }

    // A of 7 swung steps ends on an odd step of the grid. B starts there on its
    // first step, and its swing stays on the timeline's grid: steps 7, 8, 9 ...
    {
        PatternBank bank;
        fillScale (bank.slot (0), 0, { 7, 1, 0.66f, 0.5f });
        fillScale (bank.slot (1), 12, { 16, 1, 0.66f, 0.5f });
        const auto ons = runBank (bank, 441, 70000, { { 5000, 1 } });
        CHECK (ons.size() >= 10);
        for (int i = 0; i < 3 && 7 + i < (int) ons.size(); ++i)
        {
            const auto& e = ons[(size_t) (7 + i)];
            CHECK (e.note == 48 + i);
            CHECK (std::abs (e.sample - (long) std::ceil (stepStartBeats (7 + i, bank.slot (1).groove()) * 24000.0)) <= 1);
        }
    }

    // Projects from before 1.2.0 have no groove per slot: every slot gets the shared
    // one from the parameters, and the slots play exactly as 1.1.0 played them.
    {
        CHECK (Groove {}.toString() == "length 16 rate 1/16 swing 0.5 gate 0.5");
        const Groove odd { 9, 3, 0.62f, 0.3f };
        Groove parsed;
        CHECK (parsed.fromString (odd.toString()) && sameGroove (parsed, odd));
        CHECK (! parsed.fromString ("length 9 rate 1/8") && sameGroove (parsed, odd));  // incomplete: unchanged
        CHECK (! parsed.fromString ("length 9 rate 1/5 swing 0.5 gate 0.5")); // unknown rate

        PatternBank bank;
        const Groove shared { 12, 3, 0.6f, 0.4f };
        bank.slot (2).setGroove ({ 3, 0, 0.7f, 0.9f }); // whatever was there before loading
        restoreGrooves (bank, {}, shared);
        for (int i = 0; i < kNumPatternSlots; ++i)
            CHECK (sameGroove (bank.slot (i).groove(), shared));

        // a 1.2.0 project: saved grooves win, a broken one falls back to the shared one
        std::array<std::string, kNumPatternSlots> saved;
        saved[1] = odd.toString();
        saved[4] = "garbage";
        restoreGrooves (bank, saved, shared);
        CHECK (sameGroove (bank.slot (1).groove(), odd));
        CHECK (sameGroove (bank.slot (4).groove(), shared) && sameGroove (bank.slot (0).groove(), shared));

        for (const auto& groove : { Groove { 12, 3, 0.6f, 0.4f }, Groove { 7, 1, 0.66f, 0.5f }, Groove { 16, 2, 0.5f, 0.5f },
                                    Groove { 5, 4, 0.75f, 0.5f } })
        {
            PatternBank old;
            for (int slot = 0; slot < kNumPatternSlots; ++slot)
                for (int i = 0; i < kMaxSteps; ++i)
                    old.slot (slot).set (i, { i % 5 != 3, (slot * 3 + i) % 24, i % 4 == 0, false });
            restoreGrooves (old, {}, groove);
            const std::vector<Request> requests { { 20000, 1 }, { 70000, 5 }, { 71000, 2 }, { 150000, 0 }, { 200000, 7 } };
            for (int blockSize : { 64, 441, 1024 })
                CHECK (sameOns (runBank (old, blockSize, 260000, requests), playedBy110 (old, groove, blockSize, 260000, requests)));
        }
    }

    // MIDI export of all slots: each clip has its own slot's length, rate, swing and
    // gate, and plays as the sequencer plays that slot
    {
        PatternBank bank;
        const Groove grooves[kNumPatternSlots] = { { 16, 1, 0.5f, 0.5f }, { 7, 1, 0.6f, 0.5f },  { 12, 0, 0.5f, 0.8f },
                                                   { 9, 4, 0.5f, 0.5f },  { 3, 2, 0.55f, 0.2f }, { 1, 0, 0.5f, 1.0f },
                                                   { 13, 3, 0.7f, 0.5f }, { 5, 1, 0.5f, 0.05f } };
        for (int slot = 0; slot < kNumPatternSlots; ++slot)
        {
            for (int i = 0; i < kMaxSteps; ++i)
                bank.slot (slot).set (i, { i % 3 != 2, slot + i, false, false });
            bank.slot (slot).setGroove (grooves[slot]);
        }

        std::vector<PatternClip> clips;
        for (int slot = 0; slot < kNumPatternSlots; ++slot)
            clips.push_back (patternToClip (bank.slot (slot), 36));

        for (int slot = 0; slot < kNumPatternSlots; ++slot)
        {
            const auto& groove = grooves[slot];
            const auto& clip = clips[(size_t) slot];
            CHECK (std::abs (clip.lengthBeats - groove.length * groove.stepBeats()) < 1e-9);
            int gated = 0;
            for (int i = 0; i < groove.length; ++i)
                gated += i % 3 != 2 ? 1 : 0;
            CHECK ((int) clip.notes.size() == gated);
            for (const auto& n : clip.notes)
                CHECK (n.start < clip.lengthBeats && n.end <= clip.lengthBeats);

            // the sequencer playing this slot alone gives the same starts and notes
            PatternBank single;
            single.slot (0).copyFrom (bank.slot (slot));
            const auto passSamples = (long) std::lround (clip.lengthBeats * 24000.0);
            const auto ons = runBank (single, 480, passSamples, {});
            CHECK (ons.size() == clip.notes.size());
            for (size_t i = 0; i < ons.size() && i < clip.notes.size(); ++i)
                CHECK (std::abs (ons[i].sample - clip.notes[i].start * 24000.0) <= 1.0 && ons[i].note == clip.notes[i].note);
        }

        // changing one slot's groove leaves the others' clips alone
        bank.slot (1).setGroove ({ 16, 0, 0.5f, 0.5f });
        for (int slot = 0; slot < kNumPatternSlots; ++slot)
            if (slot != 1)
                CHECK (std::abs (patternToClip (bank.slot (slot), 36).lengthBeats - clips[(size_t) slot].lengthBeats) < 1e-12);
    }

    // a host's pre-roll (negative positions) is silent and the pattern starts on its
    // first step at beat 0, as without pre-roll: no glides or stuck notes before it
    {
        const long preRoll = 96000; // 4 beats at 120 bpm
        for (int blockSize : { 64, 333, 512 })
        {
            const auto fromZero = run (blockSize, settings, pattern, 4.0);
            const auto withPreRoll = run (blockSize, settings, pattern, 6.0, -1.0, -4.0);
            std::vector<Event> shifted;
            for (const auto& e : withPreRoll)
            {
                CHECK (e.sample >= preRoll);
                if (e.sample - preRoll < 4 * 48000)
                    shifted.push_back ({ e.sample - preRoll, e.on, e.note });
            }
            CHECK (! shifted.empty() && shifted.front().sample == 0 && shifted.front().on
                   && shifted.front().note == settings.rootNote + pattern.get (0).note);
            std::vector<Event> expected;
            for (const auto& e : fromZero)
                if (e.sample < 4 * 48000)
                    expected.push_back (e);
            CHECK (shifted.size() == expected.size());
            for (size_t i = 0; i < shifted.size() && i < expected.size(); ++i)
                CHECK (shifted[i].sample == expected[i].sample && shifted[i].on == expected[i].on
                       && shifted[i].note == expected[i].note);
            CHECK (openNotes (withPreRoll) == 0);
        }
    }

    return finish ("sequencer");
}
