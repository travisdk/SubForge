#!/usr/bin/env python3
# Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
# SPDX-License-Identifier: AGPL-3.0-only

"""Builds THIRD-PARTY-NOTICES.txt from the licence files that ship with the
dependencies, so no licence text is ever typed by hand.

    generate-third-party-notices.py            write THIRD-PARTY-NOTICES.txt
    generate-third-party-notices.py --check    exit 1 if the committed file is
                                               out of date with the sources

Standard library only. SDK locations default to the paths CMakeLists.txt uses.
"""

import argparse
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HOME = Path.home()

HEADER = """SubForge – Third-party notices
© 2026 Henrik Juhl

SubForge is free software under the GNU Affero General Public License v3
(see the LICENSE file). Source code: https://github.com/travisdk/SubForge
Built with JUCE, used under the AGPLv3.
VST is a registered trademark of Steinberg Media Technologies GmbH.

SubForge includes the third-party components listed below. Each section
reproduces the component's licence text exactly as it appears in the
component's own licence file.
"""


def whole_file(path):
    return path.read_text(encoding="utf-8")


def section(start_heading, end_heading):
    """The part of a text file from a heading line up to (not including) the next one."""
    def extract(path):
        lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
        start = next(i for i, line in enumerate(lines) if line.rstrip("\n") == start_heading)
        end = next(i for i, line in enumerate(lines[start + 1:], start + 1) if line.rstrip("\n") == end_heading)
        return "".join(lines[start:end])
    return extract


def components(juce, cje):
    jm = juce / "modules"
    clap_libs = cje / "clap-libs"
    return [
        ("clap-juce-extensions", "commit 7adee3a", "CLAP plug-in wrapper",
         cje / "LICENSE.md", whole_file, None),
        ("CLAP SDK", "1.2.7", "CLAP plug-in API headers",
         clap_libs / "clap" / "LICENSE", whole_file, None),
        ("clap-helpers", "commit a61bcdf", "CLAP helper classes used by the wrapper",
         clap_libs / "clap-helpers" / "LICENSE", whole_file, None),
        ("HarfBuzz", "bundled with JUCE 8.0.15", "Text shaping",
         jm / "juce_graphics" / "fonts" / "harfbuzz" / "COPYING", whole_file, None),
        ("SheenBidi", "bundled with JUCE 8.0.15", "Bidirectional text layout",
         jm / "juce_graphics" / "unicode" / "sheenbidi" / "LICENSE", whole_file, None),
        ("Independent JPEG Group libjpeg", "bundled with JUCE 8.0.15", "JPEG image decoding",
         jm / "juce_graphics" / "image_formats" / "jpglib" / "README", section("LEGAL ISSUES", "REFERENCES"),
         "This software is based in part on the work of the Independent JPEG Group."),
        ("libpng", "bundled with JUCE 8.0.15", "PNG image decoding",
         jm / "juce_graphics" / "image_formats" / "pnglib" / "LICENSE", whole_file, None),
        ("zlib", "bundled with JUCE 8.0.15", "Compression",
         jm / "juce_core" / "zip" / "zlib" / "LICENSE", whole_file, None),
        ("VST3 SDK", "bundled with JUCE 8.0.15", "VST3 plug-in format",
         jm / "juce_audio_processors_headless" / "format_types" / "VST3_SDK" / "LICENSE.txt", whole_file, None),
        ("Chakra Petch typeface", "1.0", "Labels and headings (SemiBold, Bold)",
         ROOT / "Resources" / "Fonts" / "ChakraPetch" / "OFL.txt", whole_file, None),
        ("JetBrains Mono typeface", "2.305", "Parameter values (Medium)",
         ROOT / "Resources" / "Fonts" / "JetBrainsMono" / "OFL.txt", whole_file, None),
    ]


def build(juce, cje):
    rule = "=" * 78
    parts = [HEADER]
    for name, version, use, path, extract, statement in components(juce, cje):
        if not path.is_file():
            sys.exit(f"missing licence file for {name}: {path}")
        parts.append(f"\n{rule}\n{name} ({version})\nUsed for: {use}\n{rule}\n\n")
        if statement:
            parts.append(statement + "\n\n")
        text = extract(path)
        parts.append(text if text.endswith("\n") else text + "\n")
    return "".join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--juce", type=Path, default=Path(os.environ.get("JUCE_DIR", HOME / "SDKs" / "JUCE-8.0.15")))
    parser.add_argument("--cje", type=Path,
                        default=Path(os.environ.get("CLAP_JUCE_EXTENSIONS_DIR", HOME / "SDKs" / "clap-juce-extensions-main")))
    parser.add_argument("--out", type=Path, default=ROOT / "THIRD-PARTY-NOTICES.txt")
    parser.add_argument("--check", action="store_true", help="verify the output file instead of writing it")
    args = parser.parse_args()

    text = build(args.juce, args.cje)
    if args.check:
        current = args.out.read_text(encoding="utf-8") if args.out.is_file() else ""
        if current != text:
            print(f"{args.out} is out of date: run scripts/generate-third-party-notices.py")
            return 1
        print(f"{args.out} matches the licence files")
        return 0

    args.out.write_text(text, encoding="utf-8")
    print(f"wrote {args.out} ({len(text)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
