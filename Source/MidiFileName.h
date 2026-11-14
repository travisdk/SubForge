// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once
#include <string>

namespace subforge
{
// A file name stem with only ASCII letters, digits and '-', for the MIDI clips (some hosts
// refuse spaces and special characters in a dragged file). Input is UTF-8. Danish letters
// become ae, oe and aa (Ae, Oe, Aa for capitals); every other run of characters becomes one '-'.
inline std::string midiFileStem (const std::string& utf8)
{
    std::string out;
    auto add = [&out] (const char* text) { out += text; };
    auto dash = [&out] { if (! out.empty() && out.back() != '-') out += '-'; };

    for (size_t i = 0; i < utf8.size();)
    {
        const auto b = (unsigned char) utf8[i];
        unsigned cp = b;
        size_t len = 1;
        if (b >= 0xF0)      { len = 4; cp = b & 0x07; }
        else if (b >= 0xE0) { len = 3; cp = b & 0x0F; }
        else if (b >= 0xC0) { len = 2; cp = b & 0x1F; }
        for (size_t k = 1; k < len && i + k < utf8.size(); ++k)
            cp = (cp << 6) | ((unsigned char) utf8[i + k] & 0x3F);
        i += len;

        if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || (cp >= '0' && cp <= '9'))
            out += (char) cp;
        else if (cp == 0xE6) add ("ae");
        else if (cp == 0xC6) add ("Ae");
        else if (cp == 0xF8) add ("oe");
        else if (cp == 0xD8) add ("Oe");
        else if (cp == 0xE5) add ("aa");
        else if (cp == 0xC5) add ("Aa");
        else dash();
    }
    while (! out.empty() && out.back() == '-')
        out.pop_back();
    return out;
}
} // namespace subforge
