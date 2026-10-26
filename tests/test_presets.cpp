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
    CHECK (kNumPresets == 109);

    std::set<std::string> names;
    for (const auto& preset : kPresets)
    {
        CHECK (names.insert (preset.name).second);
        const auto [rms, peak] = measureLevel (preset.values());
        CHECK (std::isfinite (rms) && std::isfinite (peak));
        const float volume = normalizedVolumeDb (preset.values());
        CHECK (volume >= -36.0f && volume <= 6.0f);
    }

    // the width is the last parameter, and no factory preset changes it
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 3]) == "squareWidth");
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 2]) == "squarePwm");
    CHECK (std::string (kPatchParamIds[kNumPatchParams - 1]) == "squareSag");
    CHECK (pSquareSag == kNumPatchParams - 1);
    for (const auto& preset : kPresets)
    {
        CHECK (preset.values()[pSquareWidth] == 0.5f);
        CHECK (preset.values()[pSquarePwm] == 0.0f);
        CHECK (preset.values()[pSquareSag] == 0.0f);
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
