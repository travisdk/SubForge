# SubForge

SubForge is a monophonic bass synthesizer with a built-in 16-step sequencer,
for Linux and Windows. It comes as a CLAP and a VST®3 plug-in, plus a
standalone app. Free and open source.

**Download the latest version:**
[**Linux** (x86-64, .tar.gz)](https://github.com/travisdk/SubForge/releases/latest/download/SubForge-linux-x86_64.tar.gz)
·
[**Windows** (64-bit, .zip)](https://github.com/travisdk/SubForge/releases/latest/download/SubForge-windows-x64.zip)
· [all releases](https://github.com/travisdk/SubForge/releases)

> **Not actively maintained.** I am not working on SubForge any more and
> share it as it is. I may fix things now and then, but there is no support
> and no promise of updates. Bug reports and pull requests are welcome, but
> may go unanswered. You are free to fork it and carry it on.

<a href="https://www.steinberg.net/"><img src="docs/images/VST_Compatible_Logo_Steinberg.png" alt="VST Compatible" width="96"></a>

![SubForge](docs/images/screenshot.png)

## Features

- Saw, square, pulse, sine and triangle waveforms, band-limited to keep
  aliasing low, plus a sine or square sub-oscillator.
- Filter with LP 24, LP 12, BP 12 and diode ladder modes, key tracking and an
  accent sweep.
- Pitch envelope, tempo-synced LFO, glide, click, distortion (overdrive,
  distortion, fuzz, tube amp) and 4x oversampling.
- 16-step sequencer with pitch, accent and slide per step, synced to the
  host's tempo.
- 89 sound presets and 42 patterns, plus your own presets and patterns.
- Vector interface that scales from 75 % to 200 %.

See [CHANGELOG.md](CHANGELOG.md) for the full list.

## Download and install

Download the ready-built plug-ins for your system (always the latest
release):

- **Linux** (x86-64):
  [SubForge-linux-x86_64.tar.gz](https://github.com/travisdk/SubForge/releases/latest/download/SubForge-linux-x86_64.tar.gz)
- **Windows** (64-bit):
  [SubForge-windows-x64.zip](https://github.com/travisdk/SubForge/releases/latest/download/SubForge-windows-x64.zip)

Older versions are on the [Releases page](https://github.com/travisdk/SubForge/releases).

Each archive contains `INSTALL.txt` with the install steps. In short, copy
`SubForge.clap` and/or the `SubForge.vst3` folder to your plug-in folder:

| | CLAP | VST3 |
|---|---|---|
| Linux | `~/.clap/` | `~/.vst3/` |
| Windows | `%LOCALAPPDATA%\Programs\Common\CLAP\` | `%LOCALAPPDATA%\Programs\Common\VST3\` |

Then rescan plug-ins in your DAW. SubForge appears as an instrument.

**Tested with:** REAPER, Bitwig Studio and Ardour on Linux. The Windows build
is made by CI and has not been tested much on real hardware.

**No macOS version.** I have no Mac to build and test on. The code is plain
JUCE and may well build on macOS, but you are on your own there.

## Building from source

You need CMake 3.22 or newer, a C++17 compiler (GCC or Clang on Linux,
Visual Studio 2022 on Windows) and two SDKs:

- [JUCE](https://github.com/juce-framework/JUCE) **8.0.15**
- [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions),
  commit `7adee3a`, with its submodules

```sh
mkdir -p ~/SDKs
git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE.git ~/SDKs/JUCE-8.0.15
git clone https://github.com/free-audio/clap-juce-extensions.git ~/SDKs/clap-juce-extensions-main
git -C ~/SDKs/clap-juce-extensions-main checkout 7adee3a1bd4684d4caa5601100e364abccff4b4d
git -C ~/SDKs/clap-juce-extensions-main submodule update --init --recursive
```

On Linux, also install the JUCE build dependencies (Debian/Ubuntu names):

```sh
sudo apt install ninja-build libasound2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev
```

Then build (on Windows, leave out `-G Ninja` and add `--config Release` to the
build and test commands):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build                        # unit tests
cmake --build build --target package-linux   # release archive in dist/ (package-windows on Windows)
```

The SDKs are looked for in `~/SDKs` (`%USERPROFILE%\SDKs` on Windows).
Use `-DJUCE_DIR=…` and `-DCLAP_JUCE_EXTENSIONS_DIR=…` to point elsewhere. By
default each build copies the plug-ins into your user plug-in folders; turn
that off with `-DSUBFORGE_INSTALL_AFTER_BUILD=OFF`.

[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) explains how the code is put
together (in Danish).

## Forks and contributions

You are welcome to fork SubForge and change it in your own repository. Only
I can change this repository: pull requests are welcome, but I decide
whether to merge them, and they may go unanswered.

If you share a modified version, the licence requires that you keep the
copyright and licence notices, mark your changes as yours, and publish your
source code under the same licence.

## Licence

Copyright © 2026 Henrik Juhl

SubForge is free software: you can redistribute it and/or modify it under
the terms of the [GNU Affero General Public License, version 3](LICENSE)
(SPDX: `AGPL-3.0-only`). It comes with **no warranty**.

SubForge is built with [JUCE](https://juce.com), used under the AGPLv3. The
other bundled components (CLAP, the VST3 SDK, HarfBuzz, the Chakra Petch and
JetBrains Mono fonts and others) are under permissive licences, listed with
their full texts in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).

VST is a registered trademark of Steinberg Media Technologies GmbH.
