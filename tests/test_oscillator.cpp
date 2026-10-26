// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Anti-aliasing: saw and square with the band-limited step, and the triangle
// with its integrated version, must be clearly closer to an additive
// band-limited reference than the naive waveforms. The same goes for the square
// at every width, which must also stay free of DC; at 50 % it must be bit for
// bit the square of earlier versions.
#include "BassSynth.h"
#include "TestUtils.h"

using namespace subforge;

constexpr double fs = 48000.0;

static double reference (int wave, double t, double hz)
{
    double s = 0.0;
    if (wave == 0) // saw: 2t - 1
    {
        for (int k = 1; k * hz < fs / 2; ++k)
            s += std::sin (2.0 * kPi * k * t) / k;
        return -2.0 / kPi * s;
    }
    if (wave == 1) // square: +1 then -1
    {
        for (int k = 1; k * hz < fs / 2; k += 2)
            s += std::sin (2.0 * kPi * k * t) / k;
        return 4.0 / kPi * s;
    }
    for (int k = 1; k * hz < fs / 2; k += 2) // triangle: -1 at 0, +1 at 0.5
        s += std::cos (2.0 * kPi * k * t) / (k * k);
    return -8.0 / (kPi * kPi) * s;
}

static double naive (int wave, double t)
{
    if (wave == 0)
        return 2.0 * t - 1.0;
    if (wave == 1)
        return t < 0.5 ? 1.0 : -1.0;
    return 1.0 - 4.0 * std::abs (t - 0.5);
}

// Pulse that is high for the first `width` of the cycle, without its DC (the oscillator removes it).
static double pulseReference (double width, double t, double hz)
{
    double s = 0.0;
    for (int k = 1; k * hz < fs / 2; ++k)
    {
        const double phi = 2.0 * kPi * k * width, theta = 2.0 * kPi * k * t;
        s += 2.0 * (std::sin (phi) * std::cos (theta) + (1.0 - std::cos (phi)) * std::sin (theta)) / (kPi * k);
    }
    return s;
}

// The square as it was before it had a width, to prove 50 % did not change.
static float squareBeforeWidth (float t, float inc)
{
    float out = t < 0.5f ? 1.0f : -1.0f;
    out += 2.0f * stepResidualAtWrap (t, inc);
    float t2 = t + 1.0f - 0.5f;
    if (t2 >= 1.0f)
        t2 -= 1.0f;
    out += -2.0f * stepResidualAtWrap (t2, inc);
    out -= 2.0f * 0.5f - 1.0f;
    return out;
}

int main()
{
    for (int wave : { 0, 1, 4 })
        for (double hz : { 1500.0, 3000.0 })
        {
            Oscillator osc;
            const int shape = wave == 4 ? 2 : wave; // reference/naive index for the triangle
            const float inc = (float) (hz / fs);
            double naiveError = 0.0, error = 0.0;
            for (int i = 0; i < 48000; ++i)
            {
                const double t = osc.phase;
                const double ref = reference (shape, t, hz);
                const double y = osc.next (inc, wave);
                if (i > 100)
                {
                    naiveError += (naive (shape, t) - ref) * (naive (shape, t) - ref);
                    error += (y - ref) * (y - ref);
                }
            }
            const double improvementDb = 10.0 * std::log10 (naiveError / error);
            std::printf ("wave %d at %.0f Hz: %.1f dB less error than naive\n", wave, hz, improvementDb);
            CHECK (improvementDb > (wave == 4 ? 2.0 : 6.0));
        }
    for (float width : { 0.15f, 0.25f, 0.35f, 0.5f, 0.65f, 0.8f, 0.9f })
        for (double hz : { 1500.0, 3000.0 })
        {
            Oscillator osc;
            const float inc = (float) (hz / fs);
            // the edges are kept two samples apart, so the highest pitches limit the width
            const double minWidth = std::min (0.5, std::max (0.1, 2.0 * inc));
            const double used = std::clamp ((double) width, minWidth, 1.0 - minWidth);
            double naiveError = 0.0, error = 0.0, sum = 0.0;
            for (int i = 0; i < 48000; ++i)
            {
                const double t = osc.phase;
                const double ref = pulseReference (used, t, hz);
                const double naivePulse = (t < used ? 1.0 : -1.0) - (2.0 * used - 1.0);
                const double y = osc.next (inc, 1, width);
                sum += y;
                if (i > 100)
                {
                    naiveError += (naivePulse - ref) * (naivePulse - ref);
                    error += (y - ref) * (y - ref);
                }
            }
            const double improvementDb = 10.0 * std::log10 (naiveError / error);
            std::printf ("square width %.2f at %.0f Hz: %.1f dB less error than naive, DC %.4f\n", width, hz, improvementDb, sum / 48000.0);
            CHECK (improvementDb > 6.0);
            CHECK (std::abs (sum / 48000.0) < 0.01);
        }

    // a width too narrow for the pitch is limited, not left to alias: 10 % at 6 kHz plays as 12.5 %
    {
        Oscillator narrow, limited;
        const float inc = (float) (6000.0 / fs);
        bool same = true;
        for (int i = 0; i < 2000; ++i)
            same = same && narrow.next (inc, 1, 0.1f) == limited.next (inc, 1, 2.0f * inc);
        CHECK (same);
    }

    // only the square has a width: the other waves ignore it
    for (int wave : { 0, 2, 3, 4 })
    {
        Oscillator a, b;
        bool same = true;
        for (int i = 0; i < 2000; ++i)
            same = same && a.next (0.01f, wave, 0.2f) == b.next (0.01f, wave);
        CHECK (same);
    }

    // 50 % is exactly the square of earlier versions, also at the highest pitches
    for (float inc : { 0.002f, 0.01f, 0.05f, 0.1f, 0.2f, 0.3f, 0.45f })
    {
        Oscillator osc;
        bool same = true;
        for (int i = 0; i < 5000; ++i)
        {
            const float expected = squareBeforeWidth (osc.phase, inc);
            same = same && osc.next (inc, 1, 0.5f) == expected;
        }
        CHECK (same);
    }

    // PWM: bipolar around the Width knob, within 10-90 % for every setting
    for (int step = 0; step <= 16; ++step)
        for (float pwm : { 0.0f, 0.3f, 1.0f })
            for (int i = 0; i <= 100; ++i)
            {
                const float width = std::clamp (0.1f + 0.05f * (float) step, 0.1f, 0.9f); // the knob's range
                const float lfo = (float) i / 100.0f;
                const float w = modulatedSquareWidth (width, pwm, lfo);
                CHECK (w >= 0.1f && w <= 0.9f);
                if (pwm == 0.0f)
                    CHECK (w == width);
            }
    CHECK (modulatedSquareWidth (0.5f, 1.0f, 0.5f) == 0.5f);
    CHECK (std::abs (modulatedSquareWidth (0.5f, 1.0f, 1.0f) - 0.9f) < 1.0e-6f);
    CHECK (std::abs (modulatedSquareWidth (0.5f, 1.0f, 0.0f) - 0.1f) < 1.0e-6f);
    CHECK (std::abs (modulatedSquareWidth (0.5f, 0.5f, 1.0f) - 0.7f) < 1.0e-6f);

    // every LFO shape stays within 0..1, which is what keeps the width inside its limits
    for (int shape = 0; shape < 5; ++shape)
    {
        Lfo lfo;
        float low = 1.0f, high = 0.0f;
        for (int i = 0; i < 200000; ++i)
        {
            const float v = lfo.next (0.0007f, shape, 0.9f);
            low = std::min (low, v);
            high = std::max (high, v);
        }
        CHECK (low >= 0.0f && high <= 1.0f);
    }

    return finish ("oscillator");
}
