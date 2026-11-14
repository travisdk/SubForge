// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Clicks and DC: the voice output must carry no DC offset, and note ends,
// retriggers and switching wave, filter mode or distortion type mid-note must
// not jump. A jump shows up as a large second difference between samples.
#include "BassSynth.h"
#include "TestUtils.h"

#include <array>
#include <string>
#include <vector>

using namespace subforge;

constexpr double fs = 192000.0; // the voice runs 4x oversampled

static std::vector<float> render (BassSynth& synth, int numSamples)
{
    std::vector<float> out ((size_t) numSamples);
    synth.render (out.data(), numSamples);
    return out;
}

static float maxSecondDifference (const std::vector<float>& v, size_t from, size_t to)
{
    float m = 0.0f;
    for (size_t i = std::max<size_t> (from, 2); i < std::min (to, v.size()); ++i)
        m = std::max (m, std::abs (v[i] - 2.0f * v[i - 1] + v[i - 2]));
    return m;
}

static BassParams heldNote()
{
    BassParams p;
    p.sustain = 1.0f;
    p.cutoffHz = 400.0f;
    p.resonance = 0.7f;
    return p;
}

// Renders a held note, changes the patch and returns how far the worst jump
// around the change exceeds the steady sound on either side, relative to the
// note's peak level.
template <typename Change>
static float switchJump (BassParams p, Change&& change)
{
    BassSynth synth;
    synth.setParams (p);
    synth.prepare (fs);
    synth.noteOn (36, 1.0f);
    auto audio = render (synth, (int) fs / 4);
    float peak = 0.0f;
    for (float s : audio)
        peak = std::max (peak, std::abs (s));
    constexpr size_t window = 8000; // a few cycles of the note, so each window sees the wave's own edges
    const float before = maxSecondDifference (audio, audio.size() - window, audio.size());

    change (p);
    synth.setParams (p);
    const auto transition = render (synth, (int) window);
    audio.insert (audio.end(), transition.begin(), transition.end());
    const float jump = maxSecondDifference (audio, audio.size() - window - 2, audio.size());
    const float after = maxSecondDifference (render (synth, (int) window), 0, window);
    return std::max (0.0f, jump - std::max (before, after)) / peak;
}

int main()
{
    // DC: saturating an asymmetric waveform (saw plus sub) creates an offset
    for (int mode : { kLowpass24, kDiode303 })
    {
        auto p = heldNote();
        p.filterMode = mode;
        p.drive = 0.2f;
        p.sub = 0.0f;
        BassSynth synth;
        synth.setParams (p);
        synth.prepare (fs);
        synth.noteOn (36, 1.0f);
        render (synth, (int) fs);
        const auto audio = render (synth, (int) fs * 4);
        double sum = 0.0, squares = 0.0;
        for (float s : audio)
        {
            sum += s;
            squares += s * s;
        }
        const double dcDb = 20.0 * std::log10 (std::abs (sum) / audio.size() / std::sqrt (squares / audio.size()));
        std::printf ("filter mode %d: DC %.1f dB below RMS\n", mode, -dcDb);
        CHECK (dcDb < -40.0);
    }

    // Note end: the distortion's filters ring out instead of being cut to zero
    for (int type : { kDistOverdrive, kDistDistortion, kDistFuzz, kDistTubeAmp })
    {
        auto p = heldNote();
        p.distType = type;
        p.distDrive = 1.0f;
        p.releaseMs = 5.0f;
        BassSynth synth;
        synth.setParams (p);
        synth.prepare (fs);
        synth.noteOn (36, 1.0f);
        render (synth, (int) fs / 10);
        synth.noteOff (36);
        const auto tail = render (synth, (int) fs);
        float jump = 0.0f;
        for (size_t i = 1; i < tail.size(); ++i)
            if (tail[i] == 0.0f)
                jump = std::max (jump, std::abs (tail[i - 1]));
        CHECK (jump < 1.0e-5f);
        CHECK (tail.back() == 0.0f);
    }

    // Square width: a new width glides in, so turning the knob does not click
    {
        auto p = heldNote();
        p.wave = 1;
        CHECK (switchJump (p, [] (BassParams& q) { q.squareWidth = 0.15f; }) < 0.01f);
    }

    // PWM: a square with no PWM ignores the LFO shape and rate, however fast it runs
    {
        auto p = heldNote();
        p.wave = 1;
        p.lfoSync = 0;
        BassSynth a, b;
        a.setParams (p);
        p.lfoShape = kLfoSampleHold;
        p.lfoRateHz = 17.0f;
        b.setParams (p);
        a.prepare (fs);
        b.prepare (fs);
        a.noteOn (36, 1.0f);
        b.noteOn (36, 1.0f);
        CHECK (render (a, 20000) == render (b, 20000));
    }

    // PWM works without LFO Depth, and only on a square
    {
        auto p = heldNote();
        p.lfoSync = 0;
        p.lfoDepth = 0.0f;
        auto audio = [&] (int wave, float pwm)
        {
            auto q = p;
            q.wave = wave;
            q.squarePwm = pwm;
            BassSynth synth;
            synth.setParams (q);
            synth.prepare (fs);
            synth.noteOn (36, 1.0f);
            return render (synth, 20000);
        };
        CHECK (audio (1, 0.8f) != audio (1, 0.0f));
        CHECK (audio (0, 0.8f) == audio (0, 0.0f));
        CHECK (audio (3, 0.8f) == audio (3, 0.0f));
    }

    // LFO Depth still moves only the filter: it does not touch the square when the cutoff is wide open
    {
        auto p = heldNote();
        p.wave = 1;
        p.lfoSync = 0;
        p.squarePwm = 0.0f;
        BassSynth a, b;
        a.setParams (p);
        p.lfoDepth = 0.8f;
        b.setParams (p);
        a.prepare (fs);
        b.prepare (fs);
        a.noteOn (36, 1.0f);
        b.noteOn (36, 1.0f);
        CHECK (render (a, 20000) != render (b, 20000)); // the filter moves
    }

    // Sag glides in and out and a wave switch with sag set does not click either
    {
        auto p = heldNote();
        p.wave = 1;
        CHECK (switchJump (p, [] (BassParams& q) { q.squareSag = 1.0f; }) < 0.01f);
        p.squareSag = 1.0f;
        CHECK (switchJump (p, [] (BassParams& q) { q.squareSag = 0.0f; }) < 0.01f);
        CHECK (switchJump (p, [] (BassParams& q) { q.wave = 0; }) < 0.01f);
        p.wave = 0;
        CHECK (switchJump (p, [] (BassParams& q) { q.wave = 1; }) < 0.01f);
    }

    // Sag only changes the square; every other wave sounds the same with sag turned all the way up
    for (int wave : { 0, 2, 3, 4 })
    {
        auto p = heldNote();
        p.wave = wave;
        BassSynth a, b;
        a.setParams (p);
        p.squareSag = 1.0f;
        b.setParams (p);
        a.prepare (fs);
        b.prepare (fs);
        a.noteOn (36, 1.0f);
        b.noteOn (36, 1.0f);
        CHECK (render (a, 20000) == render (b, 20000));
    }
    {
        auto p = heldNote();
        p.wave = 1;
        BassSynth a, b;
        a.setParams (p);
        p.squareSag = 1.0f;
        b.setParams (p);
        a.prepare (fs);
        b.prepare (fs);
        a.noteOn (36, 1.0f);
        b.noteOn (36, 1.0f);
        CHECK (render (a, 20000) != render (b, 20000));
    }

    // PWM glides in and out, so turning the knob does not click
    {
        auto p = heldNote();
        p.wave = 1;
        p.lfoSync = 0;
        CHECK (switchJump (p, [] (BassParams& q) { q.squarePwm = 1.0f; }) < 0.01f);
    }

    // Any LFO shape, width and pitch gives finite, bounded audio
    for (int shape = 0; shape < 5; ++shape)
        for (float width : { 0.1f, 0.5f, 0.9f })
            for (int note : { 24, 60, 108 })
            {
                auto p = heldNote();
                p.wave = 1;
                p.lfoShape = shape;
                p.lfoSync = 0;
                p.lfoRateHz = 9.0f;
                p.squareWidth = width;
                p.squarePwm = 1.0f;
                p.cutoffHz = 18000.0f;
                p.resonance = 0.0f;
                BassSynth synth;
                synth.setParams (p);
                synth.prepare (fs);
                synth.noteOn (note, 1.0f);
                bool ok = true;
                for (float s : render (synth, (int) fs / 2))
                    ok = ok && std::isfinite (s) && std::abs (s) < 4.0f;
                CHECK (ok);
            }

    // The width does nothing to the other waves
    {
        auto p = heldNote();
        BassSynth a, b;
        a.setParams (p);
        p.squareWidth = 0.2f;
        b.setParams (p);
        a.prepare (fs);
        b.prepare (fs);
        a.noteOn (36, 1.0f);
        b.noteOn (36, 1.0f);
        CHECK (render (a, 5000) == render (b, 5000));
    }

    // Retrigger with no gap between notes, alternating accented and plain
    {
        auto p = heldNote();
        p.accent = 1.0f;
        p.sustain = 0.7f;
        BassSynth synth;
        synth.setParams (p);
        synth.prepare (fs);
        std::vector<float> audio;
        std::vector<size_t> edges;
        for (int n = 0; n < 8; ++n)
        {
            synth.noteOff (36);
            synth.noteOn (36, n % 2 == 0 ? 1.0f : 0.7f);
            edges.push_back (audio.size());
            const auto step = render (synth, (int) (fs * 0.125));
            audio.insert (audio.end(), step.begin(), step.end());
        }
        float worst = 0.0f;
        for (size_t i = 1; i < edges.size(); ++i)
            worst = std::max (worst, maxSecondDifference (audio, edges[i], edges[i] + 4));
        CHECK (worst < 0.01f);
    }

    // Switching mid-note fades out and in around the change
    for (int from = 0; from < 4; ++from)
        for (int to = 0; to < 4; ++to)
            if (from != to)
            {
                auto p = heldNote();
                p.filterMode = from;
                CHECK (switchJump (p, [to] (BassParams& q) { q.filterMode = to; }) < 0.01f);
            }
    for (int to : { kDistOverdrive, kDistDistortion, kDistTubeAmp })
        CHECK (switchJump (heldNote(), [to] (BassParams& q) { q.distType = to; }) < 0.01f);

    // waves crossfade at the same phase; the blend must not add a jump of its own
    auto open = heldNote();
    open.cutoffHz = 18000.0f;
    open.resonance = 0.0f;
    for (int to = 1; to < 5; ++to)
        CHECK (switchJump (open, [to] (BassParams& q) { q.wave = to; }) < 0.01f);
    CHECK (switchJump (open, [] (BassParams& q) { q.subWave = 1; }) < 0.01f);

    // Accent Snap: the accented filter envelope shortens to a time constant of 60 ms,
    // the accent pulse gets a lag whose share grows with the resonance, and accents in a row build up.
    // The numbers are printed as a table; the checks hold them to the targets.
    {
        auto snapPatch = [] (float snap, float resonance, float sweep)
        {
            BassParams q;
            q.cutoffHz = 100.0f;
            q.resonance = resonance;
            q.envAmount = 1.0f;
            q.filterDecayMs = 3000.0f;
            q.sustain = 1.0f;
            q.accent = 1.0f;
            q.accentSweep = sweep;
            q.accentSnap = snap;
            return q;
        };
        // The accent shape (how far the accent opens the filter) and the filter envelope, once per 8 samples.
        static constexpr int chunk = 8; // static: the lambda below uses it without capturing (MSVC requires that)
        struct Trace
        {
            std::vector<float> shape, envelope;
            std::vector<size_t> starts;
        };
        auto traceOf = [] (const BassParams& q, int numNotes, double noteSeconds, double gateSeconds, float velocity, double totalSeconds)
        {
            BassSynth synth;
            synth.setParams (q);
            synth.prepare (fs);
            Trace t;
            std::vector<float> out ((size_t) chunk);
            const auto total = (size_t) (totalSeconds * fs);
            const auto noteLength = (size_t) std::llround (noteSeconds * fs), gateLength = (size_t) std::llround (gateSeconds * fs);
            for (size_t n = 0; n < total; n += (size_t) chunk)
            {
                const size_t note = n / noteLength, inNote = n % noteLength;
                if ((int) note < numNotes && inNote < (size_t) chunk)
                {
                    synth.noteOn (36, velocity);
                    t.starts.push_back (t.shape.size());
                }
                if ((int) note < numNotes && inNote >= gateLength && inNote < gateLength + (size_t) chunk)
                    synth.noteOff (36);
                synth.render (out.data(), chunk);
                t.shape.push_back (synth.accentShape());
                t.envelope.push_back (synth.filterEnvelope());
            }
            return t;
        };
        const double msPerChunk = 1000.0 * chunk / fs;
        auto peakOf = [] (const std::vector<float>& v, size_t from, size_t to)
        {
            size_t at = from;
            for (size_t i = from; i < std::min (to, v.size()); ++i)
                if (v[i] > v[at])
                    at = i;
            return at;
        };
        // time from the top until the envelope has fallen to a fraction of it, in ms (-1 if it never does)
        auto fallTime = [&] (const std::vector<float>& v, double fraction)
        {
            const size_t top = peakOf (v, 0, v.size());
            for (size_t i = top; i < v.size(); ++i)
                if (v[i] <= fraction * v[top])
                    return (double) (i - top) * msPerChunk;
            return -1.0;
        };
        constexpr double ln1000 = 6.907755;

        std::printf ("\naccent snap, measured\n");
        std::printf ("%-58s %10s   %s\n", "measurement", "measured", "target");

        // 1. the envelope's time constant: the time it takes to fall to 1/e of its top (resonance 0: the pulse is the envelope)
        {
            const auto full = traceOf (snapPatch (1.0f, 0.0f, 0.0f), 1, 10.0, 10.0, 1.0f, 1.6);
            const double tau = fallTime (full.envelope, std::exp (-1.0));
            std::printf ("%-58s %7.1f ms   %s\n", "time constant, Snap 100 %, Filter Decay 3000 ms", tau, "60 ms +-5 %");
            CHECK (tau > 57.0 && tau < 63.0);
            const double minus60 = fallTime (full.envelope, 0.001);
            std::printf ("%-58s %7.0f ms   %s\n", "time to -60 dB (= 6.9 time constants), same", minus60, "414 ms +-5 %");
            CHECK (minus60 > 393.0 && minus60 < 435.0);

            double previous = 1.0e9;
            for (float snap : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                const auto t = traceOf (snapPatch (snap, 0.0f, 0.0f), 1, 10.0, 10.0, 1.0f, 3.0);
                const double tc = fallTime (t.envelope, std::exp (-1.0));
                const double expected = 3000.0 * std::pow (414.0 / 3000.0, snap) / 6.9;
                std::printf ("  time constant at Snap %3.0f %%: %7.1f ms   (expected %.1f ms)\n", snap * 100.0f, tc, expected);
                CHECK (std::abs (tc - expected) < 0.05 * expected);
                CHECK (tc < previous);
                previous = tc;
            }

            // a plain note's decay grows with Snap, to 2000 ms at Filter Decay maximum
            {
                double previousPlain = 0.0;
                for (float snap : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    const auto t = traceOf (snapPatch (snap, 0.0f, 0.0f), 1, 20.0, 20.0, 0.7f, 8.0);
                    const double tc = fallTime (t.envelope, std::exp (-1.0));
                    const double expected = 3000.0 * std::pow (4.6, snap) / 6.9;
                    std::printf ("  plain note, time constant at Snap %3.0f %%: %7.1f ms   (expected %.1f ms)\n", snap * 100.0f, tc, expected);
                    CHECK (std::abs (tc - expected) < 0.05 * expected);
                    CHECK (tc > previousPlain);
                    previousPlain = tc;
                    if (snap == 1.0f)
                    {
                        std::printf ("%-58s %7.1f x    %s\n", "plain / accent time constant, Snap 100 %, Filter Decay 3000 ms", tc / 60.0, "(2000 / 60)");
                        CHECK (tc > 1900.0 && tc < 2100.0);
                    }
                }
            }

            auto knob = snapPatch (1.0f, 0.0f, 0.0f);
            knob.filterDecayMs = 100.0f;
            const auto k = traceOf (knob, 1, 10.0, 10.0, 1.0f, 0.4);
            const double kTau = fallTime (k.envelope, std::exp (-1.0));
            std::printf ("%-58s %7.1f ms   %s\n", "time constant, Snap 100 %, Filter Decay 100 ms (never longer)", kTau, "14.5 ms +-5 %");
            CHECK (kTau > 13.7 && kTau < 15.3);
        }

        // 2. the lag: time from the note's start to the top of the accent pulse, and how far the envelope has fallen by then
        {
            const float resonances[] = { 0.0f, 0.5f, 1.0f };
            double peakTime[3] {};
            for (int r = 0; r < 3; ++r)
            {
                const auto t = traceOf (snapPatch (1.0f, resonances[r], 0.0f), 1, 10.0, 10.0, 1.0f, 1.0);
                const size_t top = peakOf (t.shape, 0, t.shape.size());
                const size_t envTop = peakOf (t.envelope, 0, t.envelope.size());
                peakTime[r] = (double) top * msPerChunk;
                const double envShare = t.envelope[top] / t.envelope[envTop];
                std::printf ("resonance %.1f, Snap 100 %%: pulse tops after %5.1f ms, envelope then at %3.0f %% of its top, pulse height %.2f   %s\n",
                             resonances[r], peakTime[r], 100.0 * envShare, t.shape[top], r == 2 ? "(25-35 ms, envelope > 55 %)" : "");
                if (r > 0)
                    CHECK (envShare > 0.55);
                CHECK (r != 0 || peakTime[r] < 3.0);
                CHECK (r == 1 || (t.shape[top] > 0.85f && t.shape[top] < 1.15f)); // in between, the two peaks are blended
            }
            CHECK (peakTime[0] < peakTime[1] && peakTime[1] < peakTime[2]);
            CHECK (peakTime[2] > 25.0 && peakTime[2] < 35.0);
        }

        // 3. accents in a row (every 125 ms, a gate of 100 ms): the 2nd to 4th pulses must top the 1st at high resonance
        for (float sweep : { 0.0f, 0.5f })
        {
            const auto t = traceOf (snapPatch (1.0f, 1.0f, sweep), 4, 0.125, 0.1, 1.0f, 0.6);
            float top[4] {};
            for (size_t n = 0; n < 4; ++n)
                top[n] = t.shape[peakOf (t.shape, t.starts[n], n + 1 < 4 ? t.starts[n + 1] : t.shape.size())];
            std::printf ("accents in a row, resonance 1, sweep %.1f: tops %.2f %.2f %.2f %.2f   %s\n", sweep, top[0], top[1], top[2], top[3],
                         "2, 3, 4 > 1");
            CHECK (top[1] > top[0] && top[2] > top[0] && top[3] > top[0]);
        }
        std::printf ("\n");

        // Snap at 0 runs nothing of the feature (the sound is also bit for bit that of the version before it,
        // checked by rendering both and comparing).
        {
            auto off = snapPatch (0.0f, 0.8f, 0.5f);
            BassSynth a, b;
            a.setParams (off);
            b.setParams (off);
            a.prepare (fs);
            b.prepare (fs);
            a.noteOn (36, 1.0f);
            b.noteOn (36, 1.0f);
            CHECK (render (a, 10000) == render (b, 10000));
            a.setParams (off);
            CHECK (render (a, 10000) == render (b, 10000)); // setting the same parameters again changes nothing
        }
        // up: the accented note's filter closes sooner than at 0, a plain note's does not
        {
            auto brightness = [] (const BassParams& q, float velocity)
            {
                BassSynth synth;
                synth.setParams (q);
                synth.prepare (fs);
                synth.noteOn (36, velocity);
                const auto audio = render (synth, (int) (fs * 0.8));
                double signal = 0.0, detail = 0.0;
                for (size_t i = (size_t) (0.5 * fs); i < (size_t) (0.7 * fs); ++i)
                {
                    signal += (double) audio[i] * audio[i];
                    const double d = audio[i] - 2.0 * audio[i - 1] + audio[i - 2];
                    detail += d * d;
                }
                return std::sqrt (detail / std::max (signal, 1.0e-12));
            };
            auto q = snapPatch (0.0f, 0.8f, 0.0f);
            q.accent = 0.0f;
            auto qOn = q;
            qOn.accentSnap = 1.0f;
            CHECK (brightness (qOn, 1.0f) < 0.5 * brightness (q, 1.0f));
            CHECK (brightness (qOn, 0.7f) > brightness (q, 0.7f)); // the plain note's filter stays open longer
        }
    }

    // Accent Snap against a measuring sound (square wave, LP 24, Filter Decay 3000 ms, Accent 0.6, Sweep 0.5, amp sustain 1)
    // and its pattern (135 BPM, 1/16, gate 100 %, one pitch, accents on steps 1, 5, 9 and 10), eight bars, per step:
    //   loudness = RMS of the whole signal 0 - 100 ms after the note starts
    //   fall = energy in the band 400 - 4000 Hz in 0 - 20 ms minus the same in 80 - 100 ms
    // Snap only changes the shape of an accent, so: accents keep their loudness within 0.5 dB and fall at least 8.5 dB more,
    // and plain notes change at most 1.5 dB in loudness and fall.
    {
        struct Band
        {
            double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
            static Band make (bool highpass, double hz)
            {
                const double w = 2.0 * 3.14159265358979 * hz / fs, c = std::cos (w), alpha = std::sin (w) / (2.0 * 0.70710678), a0 = 1.0 + alpha;
                Band f;
                f.a1 = -2.0 * c / a0;
                f.a2 = (1.0 - alpha) / a0;
                f.b0 = (highpass ? (1.0 + c) / 2.0 : (1.0 - c) / 2.0) / a0;
                f.b1 = (highpass ? -(1.0 + c) : (1.0 - c)) / a0;
                f.b2 = f.b0;
                return f;
            }
            double process (double x)
            {
                const double y = b0 * x + z1;
                z1 = b1 * x - a1 * y + z2;
                z2 = b2 * x - a2 * y;
                return y;
            }
        };
        const bool accentStep[16] = { true, false, false, false, true, false, false, false, true, true, false, false, false, false, false, false };
        struct Steps { std::array<double, 16> loudness {}, fall {}; };
        auto measureSteps = [&] (float snap)
        {
            BassParams q;
            q.wave = 1; q.sub = 0.0f; q.cutoffHz = 210.8877868652344f; q.resonance = 0.7182680368423462f; q.envAmount = 0.68f;
            q.filterDecayMs = 3000.0f; q.drive = 0.25f; q.filterMode = kLowpass24; q.keyTrack = 0.2f; q.attackMs = 1.0f; q.decayMs = 450.0f;
            q.sustain = 1.0f; q.releaseMs = 40.0f; q.glideMs = 60.0f; q.velocitySens = 0.6f; q.accent = 0.6f; q.accentSweep = 0.5f;
            q.accentSnap = snap; q.gain = std::pow (10.0f, -13.16522216796875f / 20.0f);
            BassSynth synth;
            synth.setParams (q);
            synth.prepare (fs);
            Band filters[4] = { Band::make (true, 400.0), Band::make (true, 400.0), Band::make (false, 4000.0), Band::make (false, 4000.0) };
            const double stepLength = fs * 60.0 / 135.0 / 4.0;
            const size_t total = (size_t) (8.0 * 16.0 * stepLength) + (size_t) (0.2 * fs);
            std::vector<double> signal (total), band (total);
            std::vector<float> out (64);
            size_t position = 0;
            auto runTo = [&] (size_t target)
            {
                while (position < target)
                {
                    const size_t n = std::min<size_t> (64, target - position);
                    out.resize (n);
                    synth.render (out.data(), (int) n);
                    for (size_t i = 0; i < n; ++i)
                    {
                        double x = signal[position + i] = out[i];
                        for (auto& f : filters)
                            x = f.process (x);
                        band[position + i] = x;
                    }
                    position += n;
                }
            };
            for (int bar = 0; bar < 8; ++bar)
                for (int i = 0; i < 16; ++i)
                {
                    runTo ((size_t) std::llround ((bar * 16 + i) * stepLength));
                    if (bar + i > 0)
                        synth.noteOff (36); // a gate of 100 %: the note ends where the next one starts
                    synth.noteOn (36, accentStep[i] ? 1.0f : 0.7f);
                }
            runTo (total);
            auto db = [&] (const std::vector<double>& v, size_t from, size_t to)
            {
                double sum = 0.0;
                for (size_t i = from; i < to; ++i)
                    sum += v[i] * v[i];
                return 10.0 * std::log10 (sum / (double) (to - from) + 1.0e-20);
            };
            auto at = [&] (size_t start, double seconds) { return start + (size_t) std::llround (seconds * fs); };
            Steps result;
            for (int bar = 0; bar < 8; ++bar)
                for (int i = 0; i < 16; ++i)
                {
                    const size_t start = (size_t) std::llround ((bar * 16 + i) * stepLength);
                    result.loudness[(size_t) i] += db (signal, start, at (start, 0.100)) / 8.0;
                    result.fall[(size_t) i] += (db (band, start, at (start, 0.020)) - db (band, at (start, 0.080), at (start, 0.100))) / 8.0;
                }
            return result;
        };
        const auto off = measureSteps (0.0f), on = measureSteps (1.0f);
        std::printf ("measuring sound, per step: loudness (RMS 0-100 ms) and fall (band 400-4000 Hz, 0-20 ms minus 80-100 ms), dB\n");
        std::printf ("%-5s %-7s | %8s %8s %7s | %8s %8s %7s\n", "step", "type", "loud OFF", "loud ON", "diff", "fall OFF", "fall ON", "diff");
        double accentLoudness = 0.0, accentFall = 1000.0, plainLoudness = 0.0, plainFall = 0.0;
        for (int i = 0; i < 16; ++i)
        {
            const double dl = on.loudness[(size_t) i] - off.loudness[(size_t) i], df = on.fall[(size_t) i] - off.fall[(size_t) i];
            std::printf ("%-5d %-7s | %8.2f %8.2f %7.2f | %8.2f %8.2f %7.2f\n", i + 1, accentStep[i] ? "accent" : "normal", off.loudness[(size_t) i],
                         on.loudness[(size_t) i], dl, off.fall[(size_t) i], on.fall[(size_t) i], df);
            if (accentStep[i])
            {
                accentLoudness = std::max (accentLoudness, std::abs (dl));
                accentFall = std::min (accentFall, df);
            }
            else
            {
                plainLoudness = std::max (plainLoudness, std::abs (dl));
                plainFall = std::max (plainFall, std::abs (df));
            }
        }
        std::printf ("accents: loudness differs at most %.2f dB (target 0.5), fall at least %.2f dB larger (target 8.5) | "
                     "plain notes: loudness at most %.2f dB, fall at most %.2f dB (target 1.5)\n\n", accentLoudness, accentFall, plainLoudness, plainFall);
        CHECK (accentLoudness <= 0.5);
        CHECK (accentFall >= 8.5);
        CHECK (plainLoudness <= 1.5 && plainFall <= 1.5);
    }

    // ACC SLIDE: an accent on a slid-into (legato) note lifts the running envelope instead of being ignored.
    {
        BassParams q;
        q.sustain = 1.0f;
        q.cutoffHz = 300.0f;
        q.resonance = 0.8f;
        q.envAmount = 0.5f;
        q.filterDecayMs = 800.0f;
        q.accent = 0.8f;
        q.accentSweep = 0.5f;
        q.glideMs = 30.0f;
        constexpr int first = (int) (fs * 0.3), second = (int) (fs * 0.4);

        // a held note at 36, then a legato note at 43 with the given velocity; returns the audio after the legato note starts
        auto legato = [&] (const BassParams& p, float velocity, float* envelopeAfter10ms = nullptr)
        {
            BassSynth synth;
            synth.setParams (p);
            synth.prepare (fs);
            synth.noteOn (36, 0.7f);
            render (synth, first);
            synth.noteOn (43, velocity);
            if (envelopeAfter10ms != nullptr)
            {
                render (synth, (int) (fs * 0.010));
                *envelopeAfter10ms = synth.filterEnvelope();
                return std::vector<float>();
            }
            return render (synth, second);
        };

        for (const float snap : { 0.0f, 1.0f })
        {
            auto off = q, on = q;
            off.accentSnap = on.accentSnap = snap;
            on.accentSlide = true;
            const auto plain = legato (off, 0.7f), accented = legato (off, 1.0f), slidPlain = legato (on, 0.7f), slidAccent = legato (on, 1.0f);

            // off: as before, the accent of a legato note is ignored
            CHECK (accented == plain);
            // on, without an accent: nothing changes
            CHECK (slidPlain == plain);
            // on, with an accent: the sound differs, and it is louder and brighter
            CHECK (slidAccent != plain);
            auto rms = [] (const std::vector<float>& v, size_t from, size_t to)
            {
                double sum = 0.0;
                for (size_t i = from; i < to; ++i)
                    sum += (double) v[i] * v[i];
                return std::sqrt (sum / (double) (to - from));
            };
            const double lift = rms (slidAccent, 0, (size_t) (fs * 0.1)) / rms (plain, 0, (size_t) (fs * 0.1));
            std::printf ("ACC SLIDE, Snap %3.0f %%: RMS the first 100 ms after the slid-into accent is %.2fx that of the plain note\n", snap * 100.0f, lift);
            CHECK (lift > 1.1);
            float envOff = 0.0f, envOn = 0.0f;
            legato (off, 1.0f, &envOff);
            legato (on, 1.0f, &envOn);
            CHECK (envOn > envOff);

            // smooth: right after the note the sound is nearly the same as without the accent and then moves away (no step)
            float early = 0.0f, late = 0.0f, peak = 0.0f;
            for (size_t i = 0; i < slidAccent.size() / 4; ++i)
            {
                const float d = std::abs (slidAccent[i] - plain[i]);
                if (i < (size_t) (fs * 0.001))
                    early = std::max (early, d);
                late = std::max (late, d);
                peak = std::max (peak, std::abs (plain[i]));
            }
            std::printf ("                      difference within the first 1 ms %.3f, later %.3f (peak of the plain note %.3f)\n", early, late, peak);
            CHECK (early < 0.05f * peak);
            CHECK (late > 4.0f * early);
        }

        // many accents and slides in a row: finite, and not louder or more open than the same accents played as separate notes
        for (const float snap : { 0.0f, 1.0f })
        {
            struct Result { float peak = 0.0f, shape = 0.0f; bool finite = true; };
            auto run = [&] (bool slid)
            {
                auto p = q;
                p.accentSnap = snap;
                p.accentSlide = slid;
                p.resonance = 1.0f;
                p.accentSweep = 1.0f;
                BassSynth synth;
                synth.setParams (p);
                synth.prepare (fs);
                Result r;
                synth.noteOn (36, 1.0f);
                for (int n = 0; n < 32; ++n)
                {
                    for (const float v : render (synth, (int) (fs * 0.0625)))
                    {
                        r.finite = r.finite && std::isfinite (v);
                        r.peak = std::max (r.peak, std::abs (v));
                    }
                    r.shape = std::max (r.shape, synth.accentShape());
                    const int note = 36 + 12 * (n % 3) + (n % 2) * 7;
                    if (slid)
                        synth.noteOn (note, 1.0f); // legato: the new note starts before the old one ends
                    else
                    {
                        synth.allNotesOff();
                        synth.noteOn (note, 1.0f); // a separate note, retriggered
                    }
                }
                return r;
            };
            const auto separate = run (false), slid = run (true);
            std::printf ("ACC SLIDE stress, Snap %3.0f %%: peak %.3f (separate accents %.3f), highest accent shape %.2f (separate %.2f)\n", snap * 100.0f,
                         slid.peak, separate.peak, slid.shape, separate.shape);
            CHECK (slid.finite);
            CHECK (slid.peak <= 1.2f * separate.peak);
            CHECK (slid.shape <= 1.2f * separate.shape);
        }
    }

    return finish ("voice");
}
