// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// The MIDI clip file name: ASCII letters, digits and '-' only (hosts refuse more when dragging).
#include "MidiFileName.h"
#include "TestUtils.h"

using namespace subforge;

int main()
{
    CHECK (midiFileStem ("SubForge B - Chicago Jack") == "SubForge-B-Chicago-Jack");
    CHECK (midiFileStem ("  a  b  ") == "a-b");
    CHECK (midiFileStem ("Acid/Rain: 2?") == "Acid-Rain-2");
    CHECK (midiFileStem ("R\xC3\xB8""d gr\xC3\xB8""d") == "Roed-groed");           // Rød grød
    CHECK (midiFileStem ("\xC3\xA6\xC3\xB8\xC3\xA5") == "aeoeaa");                  // æøå
    CHECK (midiFileStem ("\xC3\x86\xC3\x98\xC3\x85") == "AeOeAa");                  // ÆØÅ
    CHECK (midiFileStem ("caf\xC3\xA9 \xE2\x82\xAC") == "caf");                     // é and € are dropped
    CHECK (midiFileStem ("***") == "");
    return 0;
}
