#pragma once

#include "BassSynth.h"

#include <array>
#include <random>
#include <utility>
#include <vector>

// Factory presets, the bass-aware randomizer and loudness normalisation.
namespace subforge
{

enum PatchParam
{
    pWave, pOctave, pDetune, pSub, pSubWave,
    pCutoff, pResonance, pEnvAmount, pFilterDecay, pDrive,
    pAttack, pDecay, pSustain, pRelease,
    pGlide, pVelocity,
    kNumPatchParams
};

// Parameter IDs in PatchParam order. Volume is not part of a patch: it is
// derived by normalizedVolumeDb() so every patch plays at a similar level.
inline constexpr const char* kPatchParamIds[kNumPatchParams] = {
    "wave", "octave", "detune", "sub", "subWave",
    "cutoff", "resonance", "envAmount", "filterDecay", "drive",
    "attack", "decay", "sustain", "release",
    "glide", "velocity"
};

using PatchValues = std::array<float, kNumPatchParams>;

struct Preset
{
    const char* name;
    PatchValues values;
};

// wave: 0 saw, 1 square, 2 pulse | subWave: 0 sine, 1 square
// Columns: wave oct detune sub subW | cutoff reso envAmt fDecay drive | att dec sus rel | glide vel
inline const Preset kPresets[] = {
    { "Init",              { 0,  0,  0, 0.5f, 0,   400, 0.30f, 0.50f, 250, 0.20f,   1,  600, 0.70f,  60,   50, 0.5f } },

    { "Acid Classic",      { 0,  0,  0, 0.0f, 0,   300, 0.75f, 0.60f, 200, 0.30f,   1,  400, 0.60f,  40,   60, 0.6f } },
    { "Acid Squelch",      { 0,  0,  0, 0.0f, 0,   200, 0.88f, 0.75f, 150, 0.45f,   1,  300, 0.50f,  30,   80, 0.7f } },
    { "Acid Square",       { 1,  0,  0, 0.0f, 0,   350, 0.80f, 0.55f, 250, 0.35f,   1,  400, 0.60f,  40,   70, 0.6f } },
    { "Acid Rubber",       { 0,  0,  0, 0.2f, 0,   150, 0.70f, 0.80f, 350, 0.25f,   1,  500, 0.50f,  50,  120, 0.5f } },
    { "Acid Screamer",     { 0,  0,  0, 0.0f, 0,   500, 0.92f, 0.70f, 180, 0.80f,   1,  300, 0.70f,  40,   60, 0.6f } },
    { "Acid Low Slide",    { 0, -1,  0, 0.0f, 0,   250, 0.70f, 0.65f, 300, 0.30f,   1,  500, 0.60f,  60,  150, 0.5f } },
    { "Distorted Acid",    { 0,  0,  0, 0.0f, 0,   250, 0.85f, 0.70f, 220, 1.00f,   1,  350, 0.60f,  40,   70, 0.6f } },

    { "Pure Sub",          { 0,  0,  0, 1.0f, 0,    40, 0.00f, 0.00f, 200, 0.00f,   2,  800, 0.90f,  80,    0, 0.3f } },
    { "Sub Thump",         { 0,  0,  0, 1.0f, 0,    60, 0.00f, 0.30f,  80, 0.10f,   1,  300, 0.70f,  60,    0, 0.4f } },
    { "Warm Sub",          { 1, -1,  0, 0.8f, 0,   120, 0.10f, 0.15f, 200, 0.20f,   2,  600, 0.85f, 100,   20, 0.3f } },
    { "808 Style Sub",     { 0,  0,  0, 1.0f, 0,    50, 0.00f, 0.40f,  60, 0.40f,   1, 1500, 0.00f, 800,    0, 0.4f } },
    { "Sub Square",        { 1, -1,  0, 0.6f, 1,    90, 0.05f, 0.10f, 150, 0.15f,   1,  500, 0.90f,  70,    0, 0.3f } },
    { "Soft Sub Swell",    { 0,  0,  0, 1.0f, 0,    50, 0.00f, 0.00f, 200, 0.00f,  80,  800, 0.90f, 300,   40, 0.2f } },
    { "Dub Bass",          { 0,  0,  0, 0.9f, 0,   100, 0.10f, 0.15f, 300, 0.15f,   3,  900, 0.80f, 200,   20, 0.3f } },

    { "Reese",             { 0,  0, 18, 0.4f, 0,   900, 0.15f, 0.10f, 400, 0.30f,   3,  800, 0.90f, 150,   30, 0.2f } },
    { "Dark Reese",        { 0,  0, 25, 0.5f, 0,   400, 0.20f, 0.10f, 500, 0.40f,   5,  800, 0.95f, 200,   40, 0.2f } },
    { "Reese Growl",       { 0,  0, 15, 0.3f, 1,   600, 0.50f, 0.25f, 300, 0.70f,   2,  600, 0.90f, 120,   30, 0.3f } },
    { "Wide Detune Saw",   { 0,  0, 35, 0.3f, 0,  1500, 0.10f, 0.15f, 300, 0.20f,   2,  800, 0.85f, 150,   20, 0.3f } },
    { "Neuro Grind",       { 0,  0, 22, 0.2f, 1,   800, 0.65f, 0.30f, 200, 0.95f,   1,  500, 0.90f,  80,   20, 0.4f } },
    { "Square Reese",      { 1,  0, 20, 0.4f, 0,   700, 0.20f, 0.15f, 400, 0.40f,   3,  800, 0.90f, 150,   30, 0.2f } },
    { "Hoover-ish",        { 2,  0, 30, 0.4f, 0,  1200, 0.30f, 0.25f, 400, 0.50f,  10,  800, 0.85f, 250,  120, 0.3f } },

    { "Pluck Bass",        { 0,  0,  0, 0.4f, 0,   180, 0.30f, 0.70f, 120, 0.20f,   1,  250, 0.00f,  80,    0, 0.6f } },
    { "Funk Pluck",        { 1,  0,  0, 0.3f, 0,   250, 0.45f, 0.65f,  90, 0.25f,   1,  200, 0.20f,  60,    0, 0.7f } },
    { "Short Stab",        { 0,  0,  5, 0.3f, 0,   400, 0.40f, 0.60f,  60, 0.30f,   1,  120, 0.00f,  40,    0, 0.7f } },
    { "Rubber Pluck",      { 2,  0,  0, 0.5f, 0,   200, 0.55f, 0.70f, 150, 0.20f,   1,  300, 0.10f,  70,    0, 0.6f } },
    { "Bouncy Square",     { 1,  0,  0, 0.5f, 1,   300, 0.30f, 0.50f, 180, 0.20f,   1,  350, 0.30f,  80,   30, 0.6f } },
    { "Mallet Bass",       { 2,  0,  0, 0.6f, 0,   500, 0.20f, 0.50f,  50, 0.10f,   1,  200, 0.00f, 150,    0, 0.5f } },
    { "Electro Pluck",     { 0,  0,  8, 0.2f, 1,   350, 0.60f, 0.75f, 100, 0.50f,   1,  180, 0.10f,  50,    0, 0.6f } },
    { "Muted Upright",     { 2,  0,  0, 0.5f, 0,   130, 0.10f, 0.50f,  90, 0.05f,   2,  600, 0.00f, 150,    0, 0.6f } },
    { "Soft Fingered",     { 1,  0,  0, 0.6f, 0,   160, 0.10f, 0.35f, 180, 0.05f,   3,  400, 0.40f, 120,    0, 0.6f } },

    { "Fat Moog",          { 0,  0,  7, 0.6f, 1,   500, 0.35f, 0.40f, 300, 0.30f,   2,  600, 0.70f, 100,   40, 0.4f } },
    { "Taurus Pedal",      { 0, -1,  6, 0.5f, 0,   300, 0.25f, 0.35f, 500, 0.20f,   5, 1000, 0.80f, 250,   60, 0.3f } },
    { "Minimoog Classic",  { 1,  0,  4, 0.5f, 1,   450, 0.30f, 0.45f, 250, 0.35f,   1,  500, 0.65f,  90,   30, 0.5f } },
    { "Round Bass",        { 0,  0,  0, 0.6f, 0,   220, 0.15f, 0.30f, 250, 0.10f,   2,  700, 0.75f,  90,   20, 0.4f } },
    { "Smooth Legato",     { 0,  0,  3, 0.5f, 0,   350, 0.20f, 0.25f, 400, 0.15f,   4,  800, 0.85f, 120,  150, 0.3f } },
    { "Pulse Bass",        { 2,  0,  0, 0.4f, 1,   600, 0.20f, 0.30f, 200, 0.25f,   1,  500, 0.70f,  80,    0, 0.5f } },
    { "Hollow Square",     { 1,  0,  0, 0.0f, 0,   800, 0.30f, 0.20f, 300, 0.10f,   2,  600, 0.80f,  90,    0, 0.4f } },
    { "Lo-Fi Warm",        { 1, -1,  5, 0.5f, 0,   180, 0.20f, 0.25f, 300, 0.40f,   3,  700, 0.75f, 150,   10, 0.3f } },
    { "Slow Swell",        { 0,  0, 10, 0.5f, 0,   250, 0.25f, 0.30f, 800, 0.20f, 300, 1000, 0.90f, 400,   80, 0.2f } },
    { "House Organ Bass",  { 1,  0,  0, 0.7f, 0,   600, 0.00f, 0.20f,  80, 0.10f,   1,  250, 0.50f,  60,    0, 0.5f } },

    { "Dirty Saw",         { 0,  0,  0, 0.3f, 1,  1200, 0.30f, 0.30f, 250, 0.90f,   1,  600, 0.80f,  80,   20, 0.4f } },
    { "Fuzz Bass",         { 1,  0,  0, 0.5f, 1,   900, 0.20f, 0.20f, 300, 1.00f,   1,  500, 0.85f,  80,    0, 0.4f } },
    { "Overdrive Pick",    { 0,  0,  4, 0.3f, 0,   700, 0.35f, 0.50f, 120, 0.75f,   1,  300, 0.50f,  60,    0, 0.7f } },
    { "Industrial",        { 2, -1, 12, 0.4f, 1,  1500, 0.60f, 0.40f, 150, 1.00f,   1,  400, 0.70f,  80,    0, 0.5f } },

    { "Bright Saw",        { 0,  0,  0, 0.3f, 0,  3000, 0.15f, 0.20f, 250, 0.20f,   1,  600, 0.85f,  80,    0, 0.4f } },
    { "Talking Bass",      { 0,  0,  6, 0.3f, 0,   300, 0.70f, 0.60f, 600, 0.30f,  30,  800, 0.60f, 150,  100, 0.5f } },
    { "Zap Bass",          { 1,  0,  0, 0.4f, 0,   120, 0.60f, 1.00f,  40, 0.30f,   1,  300, 0.60f,  60,    0, 0.6f } },
    { "Laser Down",        { 0,  0,  0, 0.3f, 0,    80, 0.80f, 1.00f, 250, 0.40f,   1,  400, 0.60f,  60,    0, 0.5f } },
    { "Octave Lead Bass",  { 0,  1,  6, 0.6f, 1,  1200, 0.40f, 0.35f, 300, 0.40f,   2,  600, 0.70f, 120,   90, 0.4f } },
};

inline constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

inline BassParams toBassParams (const PatchValues& v, float gain)
{
    BassParams p;
    p.wave = (int) v[pWave];
    p.octave = (int) v[pOctave];
    p.detuneCents = v[pDetune];
    p.sub = v[pSub];
    p.subWave = (int) v[pSubWave];
    p.cutoffHz = v[pCutoff];
    p.resonance = v[pResonance];
    p.envAmount = v[pEnvAmount];
    p.filterDecayMs = v[pFilterDecay];
    p.drive = v[pDrive];
    p.attackMs = v[pAttack];
    p.decayMs = v[pDecay];
    p.sustain = v[pSustain];
    p.releaseMs = v[pRelease];
    p.glideMs = v[pGlide];
    p.velocitySens = v[pVelocity];
    p.gain = gain;
    return p;
}

// Renders a held C2 offline and returns {loudest 50 ms RMS, peak}.
inline std::pair<float, float> measureLevel (const PatchValues& v)
{
    constexpr double sampleRate = 48000.0;
    constexpr int window = 2400;

    BassSynth synth;
    synth.setParams (toBassParams (v, 1.0f));
    synth.prepare (sampleRate);
    synth.noteOn (36, 0.8f);

    std::vector<float> buffer ((size_t) (sampleRate * 0.8));
    synth.render (buffer.data(), (int) buffer.size());

    float maxRms = 0.0f, peak = 0.0f;
    for (size_t start = 0; start + window <= buffer.size(); start += window / 2)
    {
        float sum = 0.0f;
        for (int i = 0; i < window; ++i)
        {
            const float s = buffer[start + (size_t) i];
            sum += s * s;
            peak = std::max (peak, std::abs (s));
        }
        maxRms = std::max (maxRms, std::sqrt (sum / window));
    }
    return { maxRms, peak };
}

inline float normalizedVolumeDb (const PatchValues& v)
{
    constexpr float targetRms = 0.2f;
    constexpr float maxPeak = 0.89f;

    const auto [rms, peak] = measureLevel (v);
    if (rms < 1.0e-6f)
        return -6.0f;

    const float gain = std::min (targetRms / rms, maxPeak / peak);
    return std::clamp (20.0f * std::log10 (gain), -36.0f, 6.0f);
}

// A new patch drawn from ranges that keep it sounding like a usable bass.
inline PatchValues randomPatch (std::mt19937& rng)
{
    std::uniform_real_distribution<float> uniform (0.0f, 1.0f);
    auto r = [&] { return uniform (rng); };
    auto chance = [&] (float p) { return r() < p; };
    auto logUniform = [&] (float lo, float hi) { return lo * std::pow (hi / lo, r()); };

    PatchValues v {};

    const float w = r();
    v[pWave] = w < 0.5f ? 0.0f : (w < 0.8f ? 1.0f : 2.0f);
    const float o = r();
    v[pOctave] = o < 0.7f ? 0.0f : (o < 0.95f ? -1.0f : 1.0f);
    v[pDetune] = chance (0.55f) ? 0.0f : 2.0f + 28.0f * r();
    v[pSub] = std::pow (r(), 0.7f);
    v[pSubWave] = chance (0.7f) ? 0.0f : 1.0f;

    v[pCutoff] = logUniform (60.0f, 2500.0f);
    v[pResonance] = 0.92f * std::pow (r(), 1.5f);
    v[pEnvAmount] = r();
    if (v[pCutoff] < 200.0f)
        v[pEnvAmount] = std::max (v[pEnvAmount], 0.35f); // keep low cutoffs from turning to mud
    v[pFilterDecay] = logUniform (40.0f, 800.0f);
    v[pDrive] = std::pow (r(), 1.3f);

    v[pAttack] = chance (0.85f) ? logUniform (0.5f, 8.0f) : logUniform (10.0f, 200.0f);
    v[pDecay] = logUniform (100.0f, 1500.0f);
    v[pSustain] = chance (0.3f) ? 0.2f * r() : 0.4f + 0.55f * r();
    v[pRelease] = logUniform (20.0f, 400.0f);

    v[pGlide] = chance (0.4f) ? 0.0f : logUniform (15.0f, 250.0f);
    v[pVelocity] = 0.2f + 0.6f * r();
    return v;
}

} // namespace subforge
