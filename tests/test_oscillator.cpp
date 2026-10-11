// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Anti-aliasing: saw and square with the band-limited step, and the triangle
// with its integrated version, must be clearly closer to an additive
// band-limited reference than the naive waveforms.
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
    return finish ("oscillator");
}
