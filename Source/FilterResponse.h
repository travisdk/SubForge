#pragma once

#include "BassSynth.h"

#include <complex>

// Analytic magnitude response of SubForge's filter for the GUI's filter curve.
// It evaluates the continuous-time models the DSP discretises (transistor ladder
// with mode taps, diode ladder with highpassed feedback) at a static cutoff and
// resonance, so the curve follows the parameters without touching the audio thread.
namespace subforge
{

// Gain (linear) at frequency hz for the given filter settings.
inline float filterMagnitude (int mode, float cutoffHz, float resonance, float hz)
{
    using C = std::complex<double>;

    if (mode == kDiode303)
    {
        // Same linear model as DiodeLadderFilter, with wc placing the resonant peak at the cutoff.
        const double wc = cutoffHz / DiodeLadderFilter::kPeakRatio;
        const C s (0.0, hz / wc);
        const double k = resonance * DiodeLadderFilter::kCriticalFeedback;

        // (s I - A) y = b u, A tridiagonal with rates {2, 1, 1, 1}
        C diag[4], lower[4], upper[4], rhs[4];
        for (int i = 0; i < 4; ++i)
        {
            const double r = DiodeLadderFilter::kRates[i];
            diag[i] = s + 2.0 * r;
            lower[i] = -r;
            upper[i] = -r;
            rhs[i] = i == 0 ? C (r) : C (0.0);
        }
        for (int i = 1; i < 4; ++i)
        {
            const C m = lower[i] / diag[i - 1];
            diag[i] -= m * upper[i - 1];
            rhs[i] -= m * rhs[i - 1];
        }
        C y = rhs[3] / diag[3];                                // y4 per unit input to the ladder
        const C highpass = C (0.0, hz) / C (60.0, hz);         // feedback highpass at 60 Hz
        y = y / (1.0 + k * highpass * y);
        return (float) (std::abs (y) / DiodeLadderFilter::kPassbandGain * (1.0 + 0.6 * resonance));
    }

    // Transistor ladder: four one-pole stages G = 1 / (1 + s), feedback from the last.
    const C s (0.0, hz / cutoffHz);
    const C G = 1.0 / (1.0 + s);
    const double k = resonance * 3.95;
    const C loop = 1.0 + k * G * G * G * G;
    C y;
    switch (mode)
    {
        case kLowpass12:   y = G * G / loop; break;
        case kBandpass12:  y = 2.0 * (G - G * G) / loop; break;
        default:           y = G * G * G * G / loop; break;
    }
    return (float) (std::abs (y) * (1.0 + 0.5 * k));
}

} // namespace subforge
