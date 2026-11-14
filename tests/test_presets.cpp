// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Factory presets and the randomizer: every patch renders finite audio and gets
// a sensible normalised volume; preset names are unique.
#include "Patches.h"
#include "TestUtils.h"

#include <set>
#include <string>

using namespace subforge;

int main()
{
    constexpr int kNumEarlierPresets = 109; // before the square shaping presets
    constexpr int kNumSquarePresetsEnd = 115; // after them come the five Accent Snap presets, which come last
    CHECK (kNumPresets == 120);

    std::set<std::string> names;
    for (const auto& preset : kPresets)
    {
        CHECK (names.insert (preset.name).second);
        const auto [rms, peak] = measureLevel (preset.values());
        CHECK (std::isfinite (rms) && std::isfinite (peak));
        const float volume = normalizedVolumeDb (preset.values());
        CHECK (volume >= -36.0f && volume <= 6.0f);
    }

    // the square shaping parameters and then Snap and Accent on Slide come last, and no earlier factory preset changes them
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 5]) == "squareWidth");
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 4]) == "squarePwm");
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 3]) == "squareSag");
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 2]) == "accentSnap");
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 1]) == "accentSlide");
    CHECK (pSquareSag == kNumPatchParams - 3 && pAccentSlide == kNumPatchParams - 1);
    for (int i = 0; i < kNumEarlierPresets; ++i)
    {
        const auto v = kPresets[i].values();
        CHECK (v[pSquareWidth] == 0.5f);
        CHECK (v[pSquarePwm] == 0.0f);
        CHECK (v[pSquareSag] == 0.0f);
    }
    // Snap is 0 and Accent on Slide off in every preset from before 1.4.0
    for (int i = 0; i < kNumSquarePresetsEnd; ++i)
    {
        const auto v = kPresets[i].values();
        CHECK (v[pAccentSnap] == 0.0f);
        CHECK (v[pAccentSlide] == 0.0f);
    }

    // the new ones are squares that use PWM or Sag, within the ranges of the knobs
    for (int i = kNumEarlierPresets; i < kNumSquarePresetsEnd; ++i)
    {
        const auto v = kPresets[i].values();
        CHECK (v[pWave] == 1.0f);
        CHECK (v[pSquarePwm] > 0.0f || v[pSquareSag] > 0.0f);
        CHECK (v[pSquareWidth] >= 0.1f && v[pSquareWidth] <= 0.9f);
        CHECK (v[pSquarePwm] >= 0.0f && v[pSquarePwm] <= 1.0f);
        CHECK (v[pSquareSag] >= 0.0f && v[pSquareSag] <= 1.0f);
        // and they play at the same level as the others, without clipping
        const auto [rms, peak] = measureLevel (v);
        const float gain = std::pow (10.0f, normalizedVolumeDb (v) / 20.0f);
        CHECK (peak * gain <= 0.9f);
        CHECK (std::abs (rms * gain - 0.2f) < 0.05f || peak * gain > 0.85f);
    }

    // the five Accent Snap presets: Snap and Accent on Slide as named, within range, and a sound at the same level as the others
    {
        struct Expected { const char* name; float snap; bool slide; };
        const Expected expected[] = { { "Snap Bite", 1.0f, false }, { "Glide Lift", 0.0f, true }, { "Rising Sweep", 0.2f, true },
                                      { "Snap Glide", 1.0f, true }, { "Half Snap", 0.4f, true } };
        for (int n = 0; n < 5; ++n)
        {
            const auto& preset = kPresets[kNumSquarePresetsEnd + n];
            CHECK (std::string (preset.name) == expected[n].name);
            const auto v = preset.values();
            CHECK (v[pAccentSnap] == expected[n].snap);
            CHECK ((v[pAccentSlide] >= 0.5f) == expected[n].slide);
            CHECK (v[pAccent] >= 0.6f && v[pSquareWidth] == 0.5f && v[pSquarePwm] == 0.0f && v[pSquareSag] == 0.0f);
            const auto [rms, peak] = measureLevel (v);
            const float gain = std::pow (10.0f, normalizedVolumeDb (v) / 20.0f);
            CHECK (peak * gain <= 0.9f);
            CHECK (std::abs (rms * gain - 0.2f) < 0.05f || peak * gain > 0.85f);
        }
        CHECK (std::string (kPresets[kNumPresets - 5].name) == "Snap Bite");
    }

    std::mt19937 rng (1);
    int clampedHigh = 0, pwmOn = 0, sagOn = 0, squares = 0, exactlyHalf = 0, slightlyOff = 0, furtherOut = 0;
    for (int i = 0; i < 2000; ++i)
    {
        const auto patch = randomPatch (rng);
        const auto [rms, peak] = measureLevel (patch);
        CHECK (std::isfinite (rms) && std::isfinite (peak));
        clampedHigh += normalizedVolumeDb (patch) >= 6.0f ? 1 : 0;

        // only a square wave gets a width other than 50 %
        // (70 % exactly 50, 20 % within 35-65, 10 % within 20-80)
        const float width = patch[pSquareWidth];
        CHECK (width >= 0.2f && width <= 0.8f);
        if (patch[pWave] != 1.0f)
        {
            CHECK (width == 0.5f);
            CHECK (patch[pSquarePwm] == 0.0f);
            CHECK (patch[pSquareSag] == 0.0f);
            continue;
        }
        ++squares;
        if (patch[pSquarePwm] != 0.0f)
        {
            CHECK (patch[pSquarePwm] >= 0.2f && patch[pSquarePwm] <= 0.7f);
            ++pwmOn;
        }
        if (patch[pSquareSag] != 0.0f)
        {
            CHECK (patch[pSquareSag] >= 0.2f && patch[pSquareSag] <= 0.8f);
            ++sagOn;
        }
        if (width == 0.5f)
            ++exactlyHalf;
        else if (width >= 0.35f && width <= 0.65f)
            ++slightlyOff;
        else
            ++furtherOut;
    }
    CHECK (squares > 300);
    CHECK (std::abs ((float) pwmOn / squares - 0.2f) < 0.06f);
    CHECK (std::abs ((float) sagOn / squares - 0.4f) < 0.08f);
    CHECK (std::abs ((float) exactlyHalf / squares - 0.7f) < 0.07f);
    // half of the widest band (20-80) lands inside 35-65, so it counts as slightly off
    CHECK (std::abs ((float) slightlyOff / squares - 0.25f) < 0.06f);
    CHECK (std::abs ((float) furtherOut / squares - 0.05f) < 0.035f);
    CHECK (clampedHigh < 40); // only rare, very quiet random patches hit the volume limit

    return finish ("presets");
}
