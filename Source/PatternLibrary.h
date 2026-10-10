#pragma once

#include "Sequencer.h"

#include <sstream>

// Factory sequencer patterns: original lines written in the classic acid
// styles (not transcriptions of existing records).
namespace subforge
{

struct FactoryPattern
{
    const char* name;
    const char* steps; // compact notation, see parseCompactPattern
    int length;
    float swing;
};

// Sixteen space-separated tokens: "-" is a rest, otherwise semitones from the
// root followed by optional "a" (accent) and/or "s" (slide), e.g. "0a 12s -".
inline bool parseCompactPattern (const char* text, Pattern& pattern)
{
    std::istringstream in (text);
    Step steps[kMaxSteps];
    for (auto& step : steps)
    {
        std::string token;
        if (! (in >> token))
            return false;

        step = {};
        if (token == "-")
        {
            step.gate = false;
            continue;
        }
        size_t used = 0;
        try
        {
            step.note = std::clamp (std::stoi (token, &used), kMinStepNote, kMaxStepNote);
        }
        catch (const std::exception&) // pasted text that isn't a pattern
        {
            return false;
        }
        for (auto c : token.substr (used))
        {
            step.accent |= c == 'a';
            step.slide |= c == 's';
        }
    }
    for (int i = 0; i < kMaxSteps; ++i)
        pattern.set (i, steps[i]);
    return true;
}

inline std::string toCompactPattern (const Pattern& pattern)
{
    std::string text;
    for (int i = 0; i < kMaxSteps; ++i)
    {
        const auto s = pattern.get (i);
        text += i > 0 ? " " : "";
        text += s.gate ? std::to_string (s.note) + (s.accent ? "a" : "") + (s.slide ? "s" : "") : "-";
    }
    return text;
}

inline const FactoryPattern kFactoryPatterns[] = {
    // Chicago acid house
    { "Chicago Jack",        "0a 0 12 0 0 12s 10 0 0a 0 12 0 3 0 12s 7",          16, 0.5f },
    { "Warehouse Squelch",   "0a - 0 0s 12 - 0 3a 0 - 0s 7 0 - 12a 10",           16, 0.5f },
    { "Basement Jack",       "0a 0s 0 12 0 0s 3 0 0a 0 12s 10 0 3 5 0",          16, 0.52f },
    { "Acid Stomp",          "0a - 0 - 0a - 0 12s 0a - 0 - 0a 3 5s 7",            16, 0.5f },
    { "Detroit Drift",       "0 7 12s 10 0 7 12a 0 0 7 12s 10 3 5 7a 0",          16, 0.5f },

    // Acid techno / free party
    { "Hammer Line",         "0a 0 0 0a 0 0 12a 0 0a 0 0 0a 0 12 0a 0",           16, 0.5f },
    { "Rolling Acid",        "0 0 0a 0 0 0 0a 12s 0 0 0a 0 0 0 0a 15",            16, 0.5f },
    { "Squat Party",         "0a 12 0 12s 0a 12 3 12 0a 12 0 12s 0a 12 5 12",     16, 0.5f },
    { "Free Party Stomp",    "0a 0 12s 12 0a 0 13s 13 0a 0 12s 12 0a 0 10 7",     16, 0.5f },
    { "Teknival",            "0a 0 0 3s 5 0 0 3a 0 0 0 7s 5 3 0 0a",              16, 0.5f },

    // Goa / psy / acid trance
    { "Goa Offbeat",         "- 0 0 0a - 0 0 0 - 0 0 0a - 0 12 0",                16, 0.5f },
    { "Psy Roller",          "- 0a 0 0 - 0a 0 0 - 0a 0 0 - 0a 0 0",               16, 0.5f },
    { "Full On Gallop",      "- 0 0 12 - 0 0 12 - 0 0 10 - 0 0 7",                16, 0.5f },
    { "Acid Trance Lead",    "0a 12 7 12 0 12 3s 5 0a 12 7 12 15s 12 10 7",       16, 0.5f },
    { "Hypnotic Spiral",     "0 3 7 12a 0 3 7 12 0 3 7 13a 0 3 7 15s",            16, 0.5f },

    // Minimal / hypnotic
    { "Minimal Pulse",       "0a - - 0 - - 0 - 0a - - 0 - 12s 0 -",               16, 0.5f },
    { "Hypnotic Loop",       "0 - 0 12s 0 - 0 3 0 - 0 12s 0 - 7 5",               16, 0.5f },
    { "Deep Pulse",          "0a - - - 0 - - 0s 0 - - - 0 - 3 -",                 16, 0.5f },
    { "Microhouse Tick",     "- 0 - 0a - 0 12 - - 0 - 0a - 0 7s 5",               16, 0.55f },

    // Electro / breaks
    { "Electro Funk",        "0a - 0 12 - 0 10 - 0a - 7 - 5 3s 0 -",              16, 0.5f },
    { "Breakbeat Acid",      "0a 0 - 12s 10 - 0 3 - 0a 0 - 7s 5 3 -",             16, 0.54f },
    { "Bass Drop",           "0a - - - - - 0 - 0a - - 12s 0 - - -",               16, 0.5f },
    { "Robot Talk",          "0 12 0 12 0a 10 0 7 0 12 0 12 0a 15s 12 10",        16, 0.5f },

    // Disco / funk
    { "Disco Octaves",       "0 12 0 12 0 12 0 12 3 15 3 15 5 17 5 17",           16, 0.5f },
    { "Funky Slap",          "0a - 12 0 - 0s 10 7 0a - 12 0 3 - 5s 7",            16, 0.58f },
    { "Boogie Walk",         "0 - 3 - 5 - 6s 7 0 - 3 - 5 - 3 -",                  16, 0.6f },

    // EBM / industrial
    { "Body Drive",          "0a 0 0 0 0a 0 0 0 3a 3 3 3 5a 5 7 7",               16, 0.5f },
    { "Body Machine",        "0a 0 12 0 0 0 12 0 0a 0 12 0 1 1 13s 1",            16, 0.5f },

    // Dub / deep
    { "Dub Steps",           "0a - - - - - 0s 7 - - 0 - - - 5 -",                 16, 0.5f },
    { "Deep House Groove",   "0 - - 0 - 12 - 0 - - 0 - 10s 7 - -",                16, 0.56f },

    // Building blocks
    { "Slide Riser",         "0s 1s 2s 3s 4s 5s 6s 7s 8s 9s 10s 11s 12s 13s 14s 15", 16, 0.5f },
    { "Octave Bounce",       "0 12 0 12 0 12 0 12 0 12 0 12 0 12 0 12",           16, 0.5f },
    { "Accent Builder",      "0 0 0 0a 0 0 0a 0 0 0a 0 0a 0a 0a 0a 0a",           16, 0.5f },
    { "Slide City",          "0s 12s 0s 10s 0s 7s 0s 5s 0s 12s 0s 15s 12s 10s 7s 5", 16, 0.5f },
    { "Phrygian Menace",     "0a 1 0 0 1s 0 0a 3 1 0 0a 1 7s 6 5 1",              16, 0.5f },
    { "Pentatonic Run",      "0 3 5 7 10 12 10 7 5 3 0a - 0 3 5s 7",              16, 0.5f },
    { "Lonely Note",         "0a - - - - - - - - - - - - - - -",                  16, 0.5f },
    { "Call & Response",     "0a 0 12 0 - - - - 3a 3 15 3 - - 5s 7",              16, 0.5f },
    { "Syncopated Bass",     "0a - - 0 - - 0 - - 0a - - 0 - 12s 10",              16, 0.5f },
    { "Rave Stab Bass",      "0a - 0a - - 0a - 0 0a - 0a - - 12a - 10",           16, 0.5f },

    // Odd lengths for polymetric loops against the 4/4 grid
    { "Twelve Step Tresillo", "0a - - 0 - - 0a - - 0 12s 10 - - - -",             12, 0.5f },
    { "Seven Wonder",        "0a 12 0 3s 5 0 7 - - - - - - - - -",                7,  0.5f },
};

inline constexpr int kNumFactoryPatterns = (int) (sizeof (kFactoryPatterns) / sizeof (kFactoryPatterns[0]));

} // namespace subforge
