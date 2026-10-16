// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Sequencer engine: timing independent of the host block size, swing,
// no stuck notes after slides, loops or stopping, and pattern slots that
// switch when the pattern starts over, and MIDI clips that play like the
// sequencer.
#include "Sequencer.h"
#include "TestUtils.h"

#include <cstdlib>
#include <vector>

using namespace subforge;

struct Event
{
    long sample;
    bool on;
    int note;
};

static std::vector<Event> run (int blockSize, SequencerSettings settings, const Pattern& pattern, double seconds,
                               double loopAtBeat = -1.0)
{
    constexpr double sampleRate = 48000.0, bpm = 120.0;
    SequencerEngine engine;
    PatternBank bank;
    bank.slot (0).copyFrom (pattern);
    std::vector<Event> events;
    double ppq = 0.0;
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
    settings.swing = 0.66f;
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
        clipSettings.swing = swing;
        Pattern line; // default line: accents, slides to other notes, a rest
        const auto clip = patternToClip (line, clipSettings);
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
        SequencerSettings clipSettings; // 1/16, gate 0.5, root C2 (36)
        Pattern p;
        p.clear();
        p.set (0, { true, 0, true, false });   // accent, gate 0.5
        p.set (1, { true, 0, false, true });   // slide to another note: overlaps it
        p.set (2, { true, 7, false, true });   // slide to the same note: one long note
        p.set (3, { true, 7, false, false });
        p.set (4, { true, 3, false, true });   // slide into a rest: ends where the rest starts
        p.set (5, { false, 0, false, false });
        p.set (15, { true, 12, false, true }); // a slide on the last step ends with the clip
        const auto clip = patternToClip (p, clipSettings);
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
        clipSettings.length = 7;
        clipSettings.stepBeats = 0.5;
        CHECK (std::abs (patternToClip (p, clipSettings).lengthBeats - 3.5) < 1e-9);
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
            SequencerSettings clipSettings;
            clipSettings.length = c.steps;
            clipSettings.stepBeats = kSeqRateBeats[c.rate];
            const auto clip = patternToClip (Pattern(), clipSettings);
            CHECK (std::abs (clip.lengthBeats - c.beats) < 1e-6);
            for (const auto& n : clip.notes)
                CHECK (n.end <= clip.lengthBeats);
        }
    }

    return finish ("sequencer");
}
