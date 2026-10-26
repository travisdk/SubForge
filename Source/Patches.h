// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

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
    kNumCoreParams,
    pFilterMode = kNumCoreParams, pKeyTrack, pAccent,
    pPitchEnv, pPitchDecay, pClick,
    pLfoShape, pLfoSync, pLfoRate, pLfoDepth, pLfoRetrig,
    pDistType, pDistDrive, pDistTone, pDistMix,
    pAccentSweep,
    pSquareWidth,
    kNumPatchParams
};

constexpr int kNumExtraParams = kNumPatchParams - kNumCoreParams;

// Parameter IDs in PatchParam order. Volume is not part of a patch: it is
// derived by normalizedVolumeDb() so every patch plays at a similar level.
inline constexpr const char* kPatchParamIds[kNumPatchParams] = {
    "wave", "octave", "detune", "sub", "subWave",
    "cutoff", "resonance", "envAmount", "filterDecay", "drive",
    "attack", "decay", "sustain", "release",
    "glide", "velocity",
    "filterMode", "keyTrack", "accent",
    "pitchEnv", "pitchDecay", "click",
    "lfoShape", "lfoSync", "lfoRate", "lfoDepth", "lfoRetrig",
    "distType", "distDrive", "distTone", "distMix",
    "accentSweep", "squareWidth"
};

using PatchValues = std::array<float, kNumPatchParams>;

// Neutral settings for the extra parameters, so presets that leave them out
// sound as before: LP24, no key track/accent/pitch env/click, LFO depth 0, distortion off.
// Columns: filterMode keyTrack accent | pitchEnv pitchDecay click | lfoShape lfoSync lfoRate lfoDepth lfoRetrig
//          | distType distDrive distTone distMix | accentSweep | squareWidth
inline constexpr std::array<float, kNumExtraParams> kExtraDefaults { 0, 0, 0, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.5f, 0.5f, 1, 0, 0.5f };

// Factory presets list their extras up to accentSweep. Parameters added later are not
// listed: every factory preset gets the default (squareWidth 50 %).
constexpr int kNumListedExtraParams = kNumExtraParams - 1;

struct Preset
{
    const char* name;
    std::array<float, kNumCoreParams> core;
    std::array<float, kNumListedExtraParams> extra = listedDefaults();

    static constexpr std::array<float, kNumListedExtraParams> listedDefaults()
    {
        std::array<float, kNumListedExtraParams> d {};
        for (int i = 0; i < kNumListedExtraParams; ++i)
            d[(size_t) i] = kExtraDefaults[(size_t) i];
        return d;
    }

    PatchValues values() const
    {
        PatchValues v {};
        std::copy (core.begin(), core.end(), v.begin());
        std::copy (kExtraDefaults.begin(), kExtraDefaults.end(), v.begin() + kNumCoreParams);
        std::copy (extra.begin(), extra.end(), v.begin() + kNumCoreParams);
        return v;
    }
};

// wave: 0 saw, 1 square, 2 pulse, 3 sine, 4 triangle | subWave: 0 sine, 1 square
// Columns: wave oct detune sub subW | cutoff reso envAmt fDecay drive | att dec sus rel | glide vel
inline const Preset kPresets[] = {
    { "Init",              { 0,  0,  0, 0.5f, 0,   400, 0.30f, 0.50f, 250, 0.20f,   1,  600, 0.70f,  60,   50, 0.5f } },

    { "Acid Classic",      { 0,  0,  0, 0.0f, 0,   300, 0.75f, 0.60f, 200, 0.30f,   1,  400, 0.60f,  40,   60, 0.6f } },
    { "Acid Squelch",      { 0,  0,  0, 0.0f, 0,   200, 0.88f, 0.75f, 150, 0.45f,   1,  300, 0.50f,  30,   80, 0.7f } },
    { "Acid Square",       { 1,  0,  0, 0.0f, 0,   350, 0.80f, 0.55f, 250, 0.35f,   1,  400, 0.60f,  40,   70, 0.6f } },
    { "Acid Rubber",       { 0,  0,  0, 0.2f, 0,   150, 0.70f, 0.80f, 350, 0.25f,   1,  500, 0.50f,  50,  120, 0.5f } },
    { "Acid Shriek",       { 0,  0,  0, 0.0f, 0,   500, 0.92f, 0.70f, 180, 0.80f,   1,  300, 0.70f,  40,   60, 0.6f } },
    { "Acid Low Slide",    { 0, -1,  0, 0.0f, 0,   250, 0.70f, 0.65f, 300, 0.30f,   1,  500, 0.60f,  60,  150, 0.5f } },
    { "Distorted Acid",    { 0,  0,  0, 0.0f, 0,   250, 0.85f, 0.70f, 220, 1.00f,   1,  350, 0.60f,  40,   70, 0.6f } },

    { "Pure Sub",          { 0,  0,  0, 1.0f, 0,    40, 0.00f, 0.00f, 200, 0.00f,   2,  800, 0.90f,  80,    0, 0.3f } },
    { "Sub Thump",         { 0,  0,  0, 1.0f, 0,    60, 0.00f, 0.30f,  80, 0.10f,   1,  300, 0.70f,  60,    0, 0.4f } },
    { "Warm Sub",          { 1, -1,  0, 0.8f, 0,   120, 0.10f, 0.15f, 200, 0.20f,   2,  600, 0.85f, 100,   20, 0.3f } },
    { "Long Sub",          { 0,  0,  0, 1.0f, 0,    50, 0.00f, 0.40f,  60, 0.40f,   1, 1500, 0.00f, 800,    0, 0.4f } },
    { "Sub Square",        { 1, -1,  0, 0.6f, 1,    90, 0.05f, 0.10f, 150, 0.15f,   1,  500, 0.90f,  70,    0, 0.3f } },
    { "Soft Sub Swell",    { 0,  0,  0, 1.0f, 0,    50, 0.00f, 0.00f, 200, 0.00f,  80,  800, 0.90f, 300,   40, 0.2f } },
    { "Dub Bass",          { 0,  0,  0, 0.9f, 0,   100, 0.10f, 0.15f, 300, 0.15f,   3,  900, 0.80f, 200,   20, 0.3f } },
    { "Sine Bass",         { 3,  0,  0, 0.3f, 0,  2000, 0.00f, 0.00f, 200, 0.10f,   2,  800, 0.85f,  90,   30, 0.4f } },
    { "Driven Sine",       { 3,  0,  0, 0.4f, 0,   600, 0.20f, 0.30f, 200, 0.70f,   1,  600, 0.80f,  80,   20, 0.5f } },
    { "Sine Pluck",        { 3,  0,  0, 0.5f, 0,   300, 0.30f, 0.60f, 100, 0.50f,   1,  250, 0.00f,  80,    0, 0.6f } },
    { "Detuned Sines",     { 3,  0, 12, 0.3f, 0,  1500, 0.00f, 0.00f, 200, 0.30f,   2,  800, 0.90f, 120,   40, 0.3f } },
    { "Triangle Bass",     { 4,  0,  0, 0.4f, 0,  1200, 0.10f, 0.15f, 200, 0.15f,   2,  700, 0.80f,  90,   20, 0.4f } },
    { "Triangle Thump",    { 4,  0,  0, 0.6f, 0,   250, 0.20f, 0.60f,  90, 0.30f,   1,  300, 0.20f,  80,    0, 0.6f } },
    { "Gritty Triangle",   { 4,  0,  5, 0.3f, 1,   800, 0.35f, 0.40f, 180, 0.75f,   1,  500, 0.75f,  80,   30, 0.5f } },

    { "Detuned Saw",       { 0,  0, 18, 0.4f, 0,   900, 0.15f, 0.10f, 400, 0.30f,   3,  800, 0.90f, 150,   30, 0.2f } },
    { "Dark Detune",       { 0,  0, 25, 0.5f, 0,   400, 0.20f, 0.10f, 500, 0.40f,   5,  800, 0.95f, 200,   40, 0.2f } },
    { "Detune Growl",      { 0,  0, 15, 0.3f, 1,   600, 0.50f, 0.25f, 300, 0.70f,   2,  600, 0.90f, 120,   30, 0.3f } },
    { "Wide Detune Saw",   { 0,  0, 35, 0.3f, 0,  1500, 0.10f, 0.15f, 300, 0.20f,   2,  800, 0.85f, 150,   20, 0.3f } },
    { "Neuro Grind",       { 0,  0, 22, 0.2f, 1,   800, 0.65f, 0.30f, 200, 0.95f,   1,  500, 0.90f,  80,   20, 0.4f } },
    { "Square Detune",     { 1,  0, 20, 0.4f, 0,   700, 0.20f, 0.15f, 400, 0.40f,   3,  800, 0.90f, 150,   30, 0.2f } },
    { "Rave Sweep",        { 2,  0, 30, 0.4f, 0,  1200, 0.30f, 0.25f, 400, 0.50f,  10,  800, 0.85f, 250,  120, 0.3f } },

    { "Pluck Bass",        { 0,  0,  0, 0.4f, 0,   180, 0.30f, 0.70f, 120, 0.20f,   1,  250, 0.00f,  80,    0, 0.6f } },
    { "Funk Pluck",        { 1,  0,  0, 0.3f, 0,   250, 0.45f, 0.65f,  90, 0.25f,   1,  200, 0.20f,  60,    0, 0.7f } },
    { "Short Stab",        { 0,  0,  5, 0.3f, 0,   400, 0.40f, 0.60f,  60, 0.30f,   1,  120, 0.00f,  40,    0, 0.7f } },
    { "Rubber Pluck",      { 2,  0,  0, 0.5f, 0,   200, 0.55f, 0.70f, 150, 0.20f,   1,  300, 0.10f,  70,    0, 0.6f } },
    { "Bouncy Square",     { 1,  0,  0, 0.5f, 1,   300, 0.30f, 0.50f, 180, 0.20f,   1,  350, 0.30f,  80,   30, 0.6f } },
    { "Mallet Bass",       { 2,  0,  0, 0.6f, 0,   500, 0.20f, 0.50f,  50, 0.10f,   1,  200, 0.00f, 150,    0, 0.5f } },
    { "Electro Pluck",     { 0,  0,  8, 0.2f, 1,   350, 0.60f, 0.75f, 100, 0.50f,   1,  180, 0.10f,  50,    0, 0.6f } },
    { "Muted Upright",     { 2,  0,  0, 0.5f, 0,   130, 0.10f, 0.50f,  90, 0.05f,   2,  600, 0.00f, 150,    0, 0.6f } },
    { "Soft Fingered",     { 1,  0,  0, 0.6f, 0,   160, 0.10f, 0.35f, 180, 0.05f,   3,  400, 0.40f, 120,    0, 0.6f } },

    { "Fat Ladder",        { 0,  0,  7, 0.6f, 1,   500, 0.35f, 0.40f, 300, 0.30f,   2,  600, 0.70f, 100,   40, 0.4f } },
    { "Deep Pedal",        { 0, -1,  6, 0.5f, 0,   300, 0.25f, 0.35f, 500, 0.20f,   5, 1000, 0.80f, 250,   60, 0.3f } },
    { "Classic Ladder",    { 1,  0,  4, 0.5f, 1,   450, 0.30f, 0.45f, 250, 0.35f,   1,  500, 0.65f,  90,   30, 0.5f } },
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

    // lfoSync: 4 = 1/4, 7 = 1/8, 9 = 1/8T, 10 = 1/16 | lfoShape: 0 sine, 1 tri, 2 saw down, 3 square, 4 S&H
    { "Wobble 1/8",        { 0,  0, 10, 0.6f, 0,    80, 0.50f, 0.00f, 200, 0.50f,   2,  800, 0.90f, 100,   20, 0.3f },
                           { 0, 0.0f, 0.0f,    0,  60, 0.00f,   0,  7, 2, 0.75f, 1,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Triplet Wobble",    { 1,  0,  8, 0.6f, 1,   100, 0.45f, 0.00f, 200, 0.60f,   2,  800, 0.90f, 100,   20, 0.3f },
                           { 0, 0.0f, 0.0f,    0,  60, 0.00f,   1,  9, 2, 0.70f, 1,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Dubstep Growl",     { 0,  0, 20, 0.5f, 1,   300, 0.60f, 0.10f, 200, 0.90f,   2,  800, 0.90f, 100,   30, 0.3f },
                           { 2, 0.0f, 0.0f,    0,  60, 0.00f,   2, 10, 2, 0.60f, 1,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Slow Wub",          { 0,  0, 15, 0.7f, 0,    70, 0.40f, 0.00f, 200, 0.40f,   5,  800, 0.95f, 150,   40, 0.3f },
                           { 0, 0.0f, 0.0f,    0,  60, 0.00f,   0,  4, 2, 0.80f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "S&H Acid",          { 0,  0,  0, 0.0f, 0,   250, 0.80f, 0.40f, 150, 0.40f,   1,  400, 0.60f,  40,   60, 0.6f },
                           { 0, 0.3f, 0.6f,    0,  60, 0.00f,   4, 10, 2, 0.50f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },

    { "Boom Sub",          { 3,  0,  0, 0.5f, 0,  2000, 0.00f, 0.00f, 200, 0.35f,   1, 1500, 0.00f, 800,    0, 0.4f },
                           { 0, 0.0f, 0.0f,   12,  60, 0.15f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Kick Bass",         { 4,  0,  0, 0.6f, 0,   600, 0.10f, 0.40f,  80, 0.40f,   1,  400, 0.00f, 120,    0, 0.5f },
                           { 0, 0.0f, 0.0f,   24,  30, 0.40f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Punchy Sub",        { 3,  0,  0, 0.8f, 0,  1000, 0.00f, 0.00f, 200, 0.10f,   1,  800, 0.85f, 100,   20, 0.3f },
                           { 0, 0.0f, 0.0f,    7,  40, 0.10f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Laser Pluck",       { 1,  0,  0, 0.3f, 0,   300, 0.50f, 0.60f, 100, 0.40f,   1,  300, 0.50f,  60,    0, 0.6f },
                           { 0, 0.0f, 0.0f,   24, 150, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Rising Growl",      { 0,  0, 12, 0.4f, 1,   500, 0.50f, 0.30f, 250, 0.70f,   3,  700, 0.85f, 120,   30, 0.4f },
                           { 0, 0.0f, 0.0f,  -12, 150, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Clicky Pluck",      { 0,  0,  0, 0.5f, 0,   200, 0.30f, 0.65f, 110, 0.25f,   1,  250, 0.00f,  80,    0, 0.6f },
                           { 0, 0.3f, 0.0f,    0,  60, 0.50f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },

    { "Acid Accent",       { 0,  0,  0, 0.0f, 0,   280, 0.78f, 0.55f, 200, 0.35f,   1,  400, 0.60f,  40,   60, 0.7f },
                           { 0, 0.3f, 0.8f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Acid 12dB",         { 0,  0,  0, 0.1f, 0,   300, 0.85f, 0.60f, 200, 0.30f,   1,  400, 0.60f,  40,   70, 0.6f },
                           { 1, 0.2f, 0.5f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Bandpass Funk",     { 1,  0,  0, 0.5f, 0,   400, 0.50f, 0.60f, 150, 0.30f,   1,  300, 0.40f,  60,    0, 0.7f },
                           { 2, 0.4f, 0.5f,    0,  60, 0.10f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Fat 12dB Ladder",   { 0,  0,  7, 0.6f, 1,   350, 0.40f, 0.45f, 300, 0.35f,   2,  600, 0.70f, 100,   40, 0.4f },
                           { 1, 0.5f, 0.3f,    0,  60, 0.05f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },
    { "Tracked Bright",    { 0,  0,  4, 0.3f, 0,   900, 0.25f, 0.25f, 250, 0.20f,   1,  600, 0.85f,  80,   20, 0.4f },
                           { 0, 1.0f, 0.0f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.0f } },

    // filterMode 3 = Diode (acid-style diode ladder). Its cutoff marks the resonant peak,
    // so it sits higher than on the other modes for a similar brightness.
    { "Diode Acid",        { 0,  0,  0, 0.0f, 0,   500, 0.75f, 0.55f, 250, 0.25f,   1, 1500, 0.90f,  15,   60, 0.6f },
                           { 3, 0.0f, 0.7f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Diode Squelch",     { 0,  0,  0, 0.0f, 0,   350, 0.92f, 0.70f, 180, 0.35f,   1, 1500, 0.90f,  15,   60, 0.7f },
                           { 3, 0.2f, 0.8f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Diode Square Acid", { 1,  0,  0, 0.0f, 0,   600, 0.85f, 0.60f, 250, 0.30f,   1, 1500, 0.90f,  15,   60, 0.6f },
                           { 3, 0.0f, 0.7f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Diode Deep Acid",   { 0, -1,  0, 0.2f, 0,   300, 0.80f, 0.50f, 400, 0.30f,   1, 1500, 0.90f,  20,   80, 0.5f },
                           { 3, 0.0f, 0.6f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Diode Shriek",      { 0,  0,  0, 0.0f, 0,   900, 0.97f, 0.65f, 200, 0.80f,   1, 1500, 0.90f,  15,   60, 0.7f },
                           { 3, 0.3f, 1.0f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Diode Rubber",      { 0,  0,  0, 0.3f, 0,   250, 0.60f, 0.80f, 350, 0.20f,   1, 1500, 0.90f,  20,  120, 0.5f },
                           { 3, 0.0f, 0.5f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Diode Wobble Acid", { 0,  0,  0, 0.0f, 0,   400, 0.85f, 0.30f, 200, 0.40f,   1, 1500, 0.90f,  15,   60, 0.6f },
                           { 3, 0.0f, 0.6f,    0,  60, 0.00f,   0, 10, 2, 0.40f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },
    { "Diode Fat",         { 0,  0,  8, 0.5f, 1,   500, 0.60f, 0.45f, 300, 0.50f,   1,  800, 0.80f,  60,   40, 0.5f },
                           { 3, 0.3f, 0.4f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   0, 0.5f, 0.5f, 1.0f,   0.6f } },

    // distType: 1 overdrive, 2 distortion, 3 fuzz, 4 tube amp
    { "Acid Thru Distortion", { 0,  0,  0, 0.0f, 0,  350, 0.90f, 0.70f, 180, 0.20f,   1, 1500, 0.90f,  15,   60, 0.7f },
                           { 3, 0.2f, 0.8f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   2, 0.60f, 0.45f, 1.0f,   0.6f } },
    { "Acid Thru Fuzz",    { 0,  0,  0, 0.0f, 0,   400, 0.85f, 0.65f, 220, 0.20f,   1, 1500, 0.90f,  15,   60, 0.7f },
                           { 3, 0.0f, 0.7f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   3, 0.70f, 0.40f, 1.0f,   0.6f } },
    { "Acid Overdriven",   { 0,  0,  0, 0.0f, 0,   300, 0.80f, 0.60f, 250, 0.25f,   1, 1500, 0.90f,  15,   60, 0.6f },
                           { 0, 0.2f, 0.7f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   1, 0.55f, 0.55f, 1.0f,   0.6f } },
    { "Acid Tube Amp",     { 1,  0,  0, 0.0f, 0,   600, 0.85f, 0.55f, 250, 0.30f,   1, 1500, 0.90f,  15,   60, 0.6f },
                           { 3, 0.0f, 0.7f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   4, 0.75f, 0.60f, 1.0f,   0.6f } },
    { "Woolly Fuzz",       { 0,  0,  0, 0.6f, 0,   800, 0.20f, 0.30f, 300, 0.10f,   2,  700, 0.85f,  80,    0, 0.4f },
                           { 0, 0.0f, 0.0f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   3, 0.60f, 0.35f, 0.705f, 0.0f } },
    { "Tube Detune",       { 0,  0, 18, 0.4f, 0,   900, 0.20f, 0.10f, 400, 0.20f,   3,  800, 0.90f, 150,   30, 0.2f },
                           { 0, 0.0f, 0.0f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   4, 0.60f, 0.50f, 1.0f,   0.0f } },
    { "Parallel Dirt Sub", { 3,  0,  0, 0.8f, 0,  1500, 0.00f, 0.00f, 200, 0.00f,   1,  800, 0.85f, 100,   20, 0.3f },
                           { 0, 0.0f, 0.0f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   2, 0.80f, 0.40f, 0.279f, 0.0f } },
    { "Crunchy Square",    { 1,  0,  0, 0.4f, 1,   500, 0.30f, 0.50f, 200, 0.20f,   1,  500, 0.70f,  80,    0, 0.6f },
                           { 1, 0.3f, 0.3f,    0,  60, 0.00f,   0,  7, 2, 0.00f, 0,   1, 0.70f, 0.65f, 1.0f,   0.0f } },

    // Acid line: resonant sawtooth and square basses, from dark and bubbling to sharp and
    // screaming. Each has a pattern of the same name.
    { "Acid - Dark Bubble",         { 0, 0, 0, 0, 0, 160, 0.74f, 0.62f, 140, 0.25f, 1, 300, 0.40f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Deep Gurgle",         { 0, -1, 0, 0.20f, 0, 180, 0.80f, 0.65f, 220, 0.25f, 1, 500, 0.60f, 40, 90, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Rubber Square",       { 1, 0, 0, 0, 0, 220, 0.76f, 0.68f, 300, 0.25f, 1, 450, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Squelch Classic",     { 0, 0, 0, 0, 0, 280, 0.80f, 0.72f, 200, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Tight Plink",         { 0, 0, 0, 0, 0, 350, 0.72f, 0.60f, 90, 0.25f, 1, 180, 0.20f, 20, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Long Sweep",          { 0, 0, 0, 0, 0, 200, 0.82f, 0.78f, 650, 0.25f, 1, 900, 0.70f, 40, 100, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Overdriven Squelch",  { 0, 0, 0, 0, 0, 260, 0.80f, 0.70f, 230, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 1, 0.35f, 0.50f, 0.40f, 0.50f } },
    { "Acid - Screaming Lead",      { 0, 0, 0, 0, 0, 480, 0.85f, 0.80f, 260, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.40f, 0.80f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.70f } },
    { "Acid - Diode Acid",          { 0, 0, 0, 0, 0, 320, 0.78f, 0.70f, 240, 0.25f, 1, 1500, 0.90f, 15, 60, 0.60f },
                                    { 3, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Hollow Square Drive", { 1, 0, 0, 0, 0, 300, 0.75f, 0.66f, 180, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 1, 0.30f, 0.45f, 0.35f, 0.50f } },
    { "Acid - Acid Snap",           { 0, 0, 0, 0, 0, 400, 0.83f, 0.75f, 110, 0.25f, 1, 220, 0.30f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.75f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Mud Pump",            { 0, -1, 0, 0.30f, 1, 150, 0.70f, 0.70f, 350, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Shriek Drive",        { 0, 0, 0, 0, 0, 450, 0.84f, 0.78f, 200, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.80f, 0, 60, 0, 0, 7, 2, 0, 0, 1, 0.45f, 0.60f, 0.50f, 0.50f } },
    { "Acid - Warm Overdrive",      { 0, 0, 0, 0, 0, 220, 0.72f, 0.62f, 400, 0.25f, 1, 700, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 1, 0.25f, 0.35f, 0.30f, 0.50f } },
    { "Acid - Twelve Bounce",       { 0, 0, 0, 0, 0, 260, 0.79f, 0.70f, 170, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Twelve Square Roll",  { 1, 0, 0, 0, 0, 340, 0.80f, 0.72f, 150, 0.25f, 1, 500, 0.60f, 40, 80, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Twelve Driven",       { 0, 0, 0, 0, 0, 300, 0.82f, 0.68f, 280, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 1, 0.40f, 0.50f, 0.45f, 0.50f } },
    { "Acid - Seven Wobble",        { 0, 0, 0, 0, 0, 240, 0.80f, 0.74f, 190, 0.25f, 1, 500, 0.60f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.70f } },
    { "Acid - Seven Square Stab",   { 1, 0, 0, 0, 0, 420, 0.78f, 0.64f, 120, 0.25f, 1, 250, 0.30f, 40, 60, 0.60f },
                                    { 0, 0.20f, 0.60f, 0, 60, 0, 0, 7, 2, 0, 0, 0, 0.50f, 0.50f, 1, 0.50f } },
    { "Acid - Acid Shrieker",       { 0, 0, 0, 0, 0, 500, 0.85f, 0.80f, 320, 0.25f, 1, 1500, 0.90f, 15, 60, 0.60f },
                                    { 3, 0.20f, 0.85f, 0, 60, 0, 0, 7, 2, 0, 0, 1, 0.40f, 0.65f, 0.40f, 0.80f } },
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
    p.filterMode = (int) v[pFilterMode];
    p.keyTrack = v[pKeyTrack];
    p.accent = v[pAccent];
    p.pitchEnvSemis = v[pPitchEnv];
    p.pitchEnvDecayMs = v[pPitchDecay];
    p.click = v[pClick];
    p.lfoShape = (int) v[pLfoShape];
    p.lfoSync = (int) v[pLfoSync];
    p.lfoRateHz = v[pLfoRate];
    p.lfoDepth = v[pLfoDepth];
    p.lfoRetrigger = v[pLfoRetrig] >= 0.5f;
    p.distType = (int) v[pDistType];
    p.distDrive = v[pDistDrive];
    p.distTone = v[pDistTone];
    p.distMix = v[pDistMix];
    p.accentSweep = v[pAccentSweep];
    p.squareWidth = v[pSquareWidth];
    p.gain = gain;
    return p;
}

// Renders a held, unaccented C2 offline and returns {loudest 50 ms RMS, peak}.
inline std::pair<float, float> measureLevel (const PatchValues& v)
{
    constexpr double sampleRate = 48000.0;
    constexpr int window = 2400;

    BassSynth synth;
    synth.setParams (toBassParams (v, 1.0f));
    synth.prepare (sampleRate);
    synth.noteOn (36, 0.75f);

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
    v[pWave] = w < 0.4f ? 0.0f : (w < 0.64f ? 1.0f : (w < 0.77f ? 2.0f : (w < 0.88f ? 3.0f : 4.0f)));
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

    const float m = r();
    int mode = m < 0.55f ? kLowpass24 : (m < 0.7f ? kLowpass12 : (m < 0.8f ? kBandpass12 : kDiode303));
    if (mode == kBandpass12)
    {
        // bandpass removes the fundamental, so it needs harmonics to work with
        if (v[pWave] >= 3.0f)
            mode = kLowpass24;
        else
            v[pCutoff] = std::clamp (v[pCutoff], 150.0f, 700.0f);
    }
    else if (mode == kDiode303)
    {
        v[pCutoff] = logUniform (200.0f, 1500.0f); // diode cutoff marks the resonant peak, so it sits higher
    }
    v[pFilterMode] = (float) mode;
    v[pKeyTrack] = chance (0.5f) ? 0.0f : 0.6f * r();
    v[pAccent] = chance (0.4f) ? 0.0f : 0.3f + 0.7f * r();

    v[pPitchEnv] = 0.0f;
    if (chance (0.35f))
        v[pPitchEnv] = chance (0.7f) ? logUniform (2.0f, 24.0f) : -logUniform (1.0f, 12.0f);
    v[pPitchDecay] = logUniform (10.0f, 200.0f);
    v[pClick] = chance (0.7f) ? 0.0f : 0.1f + 0.5f * r();

    // LFO stays off most of the time; when on it is usually a synced wobble
    static constexpr int wobbleSyncs[] = { 4, 6, 7, 8, 9, 10 };
    v[pLfoShape] = (float) std::uniform_int_distribution<int> (0, 4) (rng);
    v[pLfoSync] = chance (0.8f) ? (float) wobbleSyncs[std::uniform_int_distribution<int> (0, 5) (rng)] : 0.0f;
    v[pLfoRate] = logUniform (0.5f, 8.0f);
    v[pLfoDepth] = chance (0.75f) ? 0.0f : 0.2f + 0.6f * r();
    v[pLfoRetrig] = chance (0.5f) ? 1.0f : 0.0f;

    v[pDistType] = chance (0.65f) ? (float) kDistOff : (float) std::uniform_int_distribution<int> (1, 4) (rng);
    v[pDistDrive] = 0.2f + 0.7f * r();
    v[pDistTone] = 0.3f + 0.5f * r();
    v[pDistMix] = 0.6f + 0.4f * r();
    v[pAccentSweep] = v[pAccent] > 0.0f ? 0.8f * r() : 0.0f;

    // only a square wave has a width; mostly it stays at 50 %
    v[pSquareWidth] = 0.5f;
    if (v[pWave] == 1.0f && chance (0.3f))
        v[pSquareWidth] = 0.2f + 0.6f * r();
    return v;
}

} // namespace subforge
