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

    std::mt19937 rng (1);
    int clampedHigh = 0;
    for (int i = 0; i < 2000; ++i)
    {
        const auto patch = randomPatch (rng);
        const auto [rms, peak] = measureLevel (patch);
        CHECK (std::isfinite (rms) && std::isfinite (peak));
        clampedHigh += normalizedVolumeDb (patch) >= 6.0f ? 1 : 0;
    }
    CHECK (clampedHigh < 40); // only rare, very quiet random patches hit the volume limit

    return finish ("presets");
}
