# Changelog

All notable changes to SubForge are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and version numbers
follow [Semantic Versioning](https://semver.org/).

When releasing: move the entries under "Unreleased" into a new version
section, raise `project(SubForge VERSION …)` in `CMakeLists.txt` to match and
tag the commit (`vX.Y.Z`).

## [Unreleased]

### Fixed
- The output no longer carries a DC offset. The saturating filter input made
  one from asymmetric waveforms, even with the distortion off.
- No click at the end of a note when the distortion is on. Its filters now
  ring out instead of being cut off.
- No click when a note restarts while the previous one is still sounding
  with a different velocity or accent, as in sequencer lines at 100 % gate.
- Volume, Drive, Sub, Resonance and the distortion knobs glide instead of
  stepping, so automating them no longer crackles.
- Switching the filter mode, distortion type or waveform during a note no
  longer clicks.

### Changed
- Every source file now carries a copyright and licence notice
  (`AGPL-3.0-only`), and the README explains the rules for forks.
- The release archives are named without a version
  (`SubForge-linux-x86_64.tar.gz`, `SubForge-windows-x64.zip`), so the README
  can link straight to the latest download. The folder inside still has the
  version.

### Removed
- The standalone app is no longer built. It was never in the release
  archives, and the sequencer could not run in it without a host transport.

## [1.0.0] – 2026-10-11

First public release, as free software under the GNU AGPLv3.

### Sound
- Monophonic bass synth with saw, square, pulse, sine and triangle
  waveforms, band-limited to keep aliasing low, plus a sine or square
  sub-oscillator.
- Filter with LP 24, LP 12, BP 12 and diode ladder modes, key tracking and an
  accent sweep.
- Pitch envelope, tempo-synced LFO, click, glide and velocity sensitivity.
- Distortion stage with overdrive, distortion, fuzz and tube amp types.
- 4x oversampling.
- Mod wheel and aftertouch routing.
- Random and mutate buttons for new sounds.
- 89 sound presets, plus user presets you can save, browse and delete in a
  folder of your choice.

### Sequencer
- 16-step sequencer with pitch, accent and slide per step, synced to the
  host's tempo.
- 42 factory patterns, user patterns, copy and paste, and a random pattern
  generator.
- Sound presets and patterns are kept separate, so you can change one without
  the other.
- The sequencer steps can be hidden to make the window smaller.

### Interface
- Vector interface that scales from 75 % to 200 % from the gear menu or by
  dragging the corner, and is shrunk if needed to fit the screen.
- The chosen size is remembered separately for each DAW, since DAWs scale
  plug-in windows differently.
- Live filter response curve, sequencer playhead and selectable accent colour.
- "About SubForge" panel with the third-party notices.

### Formats and platforms
- CLAP and VST®3 plug-ins for Linux (x86-64) and Windows (x64). No macOS
  version.
- The Windows build includes the Visual C++ runtime, so nothing extra has to
  be installed.

<img src="docs/images/VST_Compatible_Logo_Steinberg.png" alt="VST Compatible" width="72">

VST is a registered trademark of Steinberg Media Technologies GmbH.

[Unreleased]: https://github.com/travisdk/SubForge/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/travisdk/SubForge/releases/tag/v1.0.0
