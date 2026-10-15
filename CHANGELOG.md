# Changelog

All notable changes to SubForge are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and version numbers
follow [Semantic Versioning](https://semver.org/).

When releasing: move the entries under "Unreleased" into a new version
section, raise `project(SubForge VERSION …)` in `CMakeLists.txt` to match and
tag the commit (`vX.Y.Z`).

## [Unreleased]

### Added
- Tooltips on all controls. Each one explains in a few words what the
  control does. **Settings > Show tooltips** turns them off; the choice is
  saved for all projects.
- Settings and About show the exact build version (for example
  `1.0.1-3-g945938c` for a build between releases).

### Changed
- The settings button is now drawn as a gear. The old icon looked like a sun
  and could be mistaken for a theme switch.

## [1.0.1] – 2026-10-14

### Fixed
- The distortion stage no longer drops the level when you turn it on. Its
  automatic level compensation was set for a much quieter signal, so all four
  types came out quieter than the dry sound as the Gain knob went up: up to
  about 5 dB in typical playing, and more when played hard. The compensation
  is now set for typical playing (a held note at medium velocity with the
  Init patch), where all types stay within 1 dB of the dry sound across the
  Gain knob. Distortion evens out the dynamics, so with the distortion on,
  soft playing comes out a little louder than the dry sound (about 2–3 dB at
  low velocity) and hard playing a little quieter (up to about 4 dB at full
  velocity and full Sustain).

### Changed
- **Projects and user presets that use distortion play louder.** Projects
  saved with 1.0.0, and your own saved presets, keep their Volume setting.
  With the distortion on, they now play up to about 5 dB louder than before
  (at the default Gain: Overdrive +2.9 dB, Distortion and Fuzz +4.4 dB, Tube
  Amp +2.2 dB; at very low Gain settings slightly quieter instead), and peaks
  rise by the same amount. Turn Volume down by that amount, or reselect a
  factory preset, to get the old level back.
- Factory presets sound exactly as before when you load them. Woolly Fuzz and
  Parallel Dirt Sub have a lower Mix to keep their old blend. In projects
  saved with these two, the distortion will be stronger as well as louder, so
  reload the preset to get the old sound.
- Projects now save a state version, so later releases can adapt older
  projects when needed.

### Known issues
- Fuzz: the Tone control changes the output level, and at high Tone settings
  short peaks can exceed 0 dBFS. Lower Volume if needed. This may be improved
  in a later version.

## [1.0.0] – 2026-10-13

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
- The sequencer section can be collapsed to make the window smaller. Its
  on/off switch stays visible.

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
- Plug-ins only. There is no standalone app, since the sequencer follows the
  host transport.

<img src="docs/images/VST_Compatible_Logo_Steinberg.png" alt="VST Compatible" width="72">

VST is a registered trademark of Steinberg Media Technologies GmbH.

[Unreleased]: https://github.com/travisdk/SubForge/compare/v1.0.1...HEAD
[1.0.1]: https://github.com/travisdk/SubForge/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/travisdk/SubForge/releases/tag/v1.0.0
