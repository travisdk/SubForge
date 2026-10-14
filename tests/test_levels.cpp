// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Levels: switching the distortion on must not change the loudness at a typical
// playing point. Every type, across the Gain knob and at full and half Mix,
// stays within 1 dB of the dry voice, measured as K-weighted (BS.1770) loudness
// of the Init patch (Sustain 0.7) with a 5 s held note at velocity 100. This is
// the point Distortion::makeupGain() is calibrated at; softer playing comes out
// louder than dry and harder playing quieter, since the distortion evens out
// the dynamics.
#include "BassSynth.h"
#include "TestUtils.h"

#include <cmath>
#include <vector>

using namespace subforge;

constexpr double fs = 192000.0; // the voice runs 4x oversampled

struct Biquad
{
    double b0, b1, b2, a1, a2, z1 = 0.0, z2 = 0.0;

    double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

// Mean square after the BS.1770 K-weighting (high shelf, then highpass), designed
// for any sample rate. Over the whole note, like integrated loudness: the amp
// envelope never falls below the gate.
static double kWeightedPower (const std::vector<float>& audio)
{
    double K = std::tan (kPi * 1681.974450955533 / fs), Q = 0.7071752369554196;
    const double vh = std::pow (10.0, 3.999843853973347 / 20.0), vb = std::pow (vh, 0.4996667741545416);
    double a0 = 1.0 + K / Q + K * K;
    Biquad shelf { (vh + vb * K / Q + K * K) / a0, 2.0 * (K * K - vh) / a0, (vh - vb * K / Q + K * K) / a0,
                   2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
    K = std::tan (kPi * 38.13547087602444 / fs);
    Q = 0.5003270373238773;
    a0 = 1.0 + K / Q + K * K;
    Biquad highpass { 1.0, -2.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };

    double sum = 0.0;
    for (float x : audio)
    {
        const double y = highpass.process (shelf.process (x));
        sum += y * y;
    }
    return sum / (double) audio.size();
}

static double loudnessDb (int type, float drive, float mix, int note)
{
    BassParams p; // Init
    p.distType = type;
    p.distDrive = drive;
    p.distMix = mix;
    BassSynth synth;
    synth.setParams (p);
    synth.prepare (fs);
    synth.noteOn (note, 100.0f / 127.0f);
    std::vector<float> audio ((size_t) fs * 5);
    synth.render (audio.data(), (int) audio.size());
    return 10.0 * std::log10 (kWeightedPower (audio));
}

int main()
{
    double worst = 0.0;
    for (int note : { 36, 48 })
    {
        const double dry = loudnessDb (kDistOff, 0.5f, 1.0f, note);
        for (int type : { kDistOverdrive, kDistDistortion, kDistFuzz, kDistTubeAmp })
            for (float mix : { 1.0f, 0.5f })
                for (float drive : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    const double diff = loudnessDb (type, drive, mix, note) - dry;
                    worst = std::max (worst, std::abs (diff));
                    if (std::abs (diff) > 1.0)
                        std::printf ("note %d type %d drive %.2f mix %.1f: %+.1f dB\n", note, type, drive, mix, diff);
                    CHECK (std::abs (diff) <= 1.0);
                }
    }

    std::printf ("largest difference from dry: %.1f dB\n", worst);
    return finish ("levels");
}
