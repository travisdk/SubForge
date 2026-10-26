// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Clicks and DC: the voice output must carry no DC offset, and note ends,
// retriggers and switching wave, filter mode or distortion type mid-note must
// not jump. A jump shows up as a large second difference between samples.
#include "BassSynth.h"
#include "TestUtils.h"

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

    return finish ("voice");
}
