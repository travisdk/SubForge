// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <cstdio>

// Minimal test helpers: CHECK records a failure and keeps going, finish()
// prints a summary and returns the process exit code for CTest.
inline int& testFailures()
{
    static int failures = 0;
    return failures;
}

#define CHECK(condition)                                                                   \
    do                                                                                     \
    {                                                                                      \
        if (! (condition))                                                                 \
        {                                                                                  \
            ++testFailures();                                                              \
            std::printf ("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);              \
        }                                                                                  \
    } while (false)

inline int finish (const char* name)
{
    std::printf ("%s: %s (%d failed checks)\n", name, testFailures() == 0 ? "passed" : "FAILED", testFailures());
    return testFailures() == 0 ? 0 : 1;
}
