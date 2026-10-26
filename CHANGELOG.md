# Changelog

All notable changes to SubForge are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and version numbers
follow [Semantic Versioning](https://semver.org/).

When releasing: move the entries under "Unreleased" into a new version
section, raise `project(SubForge VERSION …)` in `CMakeLists.txt` to match and
tag the commit (`vX.Y.Z`).

## [Unreleased]

### Added
- **Square Width.** A new Width knob in the OSC panel sets how long the square
  wave stays high, from 10 % to 90 %. 50 % is the plain square, so existing
  projects and presets sound exactly as before. The knob is greyed out unless
  the wave is SQR. Randomize now sometimes picks another width for a square
  wave, and Mutate nudges it.

## [1.2.2] – 2026-10-22

### Changed
- CI pins JUCE and the GitHub actions to exact commits.
- Third-party components are listed in a new `THIRD-PARTY-NOTICES.md`.

### Fixed
- **Sturdier loading.** Damaged or hostile projects, sound presets,
  pattern files and pasted pattern text are handled safely: values that are
  not numbers load as defaults, and states or files that are far too large
  or deeply nested to be real are refused instead of stalling or crashing
  the host.
- On Linux, opening the preset, pattern or MIDI folder no longer passes the
  folder path through a shell.
- A root note typed in the host with a huge octave is limited to MIDI's
  range instead of overflowing.
- A preset or pattern name with no legal characters (such as "///") is no
  longer saved as a hidden file.
- After a project loads, the host is told to re-read all parameter values
  (CLAP), and a reloaded state saves exactly as it was loaded. Passes
  clap-validator and pluginval state checks.

## [1.2.1] – 2026-10-17

### Changed
- The README now lists a known limitation: switching slots while the DAW
  records automation also records Steps, Rate, Swing and Gate.
- Pattern-slot notes are now given as MIDI note numbers 0-7 in the README,
  the slot tooltip and the Settings menu (**MIDI notes 0-7 choose pattern**),
  with examples of what DAWs call them: C-2 to G-2 in Bitwig and Ableton
  Live, C0 to G0 in FL Studio. The name C-2 only fits some DAWs.

### Fixed
- **No gliding notes during a pre-roll.** With a count-in before bar 1
  (Bitwig's pre-roll, for example), the sequencer played every note as a
  slide into the next. It now stays silent before bar 1 and starts there
  on its first step, as without a pre-roll.

## [1.2.0] – 2026-10-16

### Added
- **20 acid presets.** "Acid - …" sound presets, from dark and bubbling
  to sharp and screaming, each with a sequencer pattern of the same name
  (mostly 16 steps, a few with 12 or 7). They come after the existing
  presets and patterns, so projects saved with earlier versions load the
  same presets as before.

### Changed
- **Each pattern slot has its own Steps, Rate, Swing and Gate.** Before,
  the four were shared by all slots, so loading a 7-step pattern into B
  made A play only 7 steps too. Now a slot holds everything a pattern file
  holds. The controls, and automation of them, change the slot that plays.
  A new slot starts on its first step with its own length and rate, and
  **Export all slots as MIDI** gives each clip its own length. Projects
  from 1.1.0 give every slot the settings they had and play as before,
  unless they automate one of the four and switch slots: that automation
  now changes only the slot that plays. The root note is still shared by
  all slots, and its tooltip now says so.

## [1.1.0] – 2026-10-15

### Added
- **Pattern slots A-H.** The sequencer holds eight patterns, saved with the
  project, and plays one at a time. Choose the slot with the A-H buttons next
  to the steps, by automating the new **Sequencer Pattern** parameter, or by
  playing MIDI notes C-2 to G-2 while the sequencer runs (those notes no
  longer transpose; **Settings > MIDI C-2 to G-2 choose pattern** turns this
  off). A new slot starts when the playing pattern starts over, so a switch
  always lands on the loop. The steps show and edit the slot that plays.
  **Copy pattern to slot** in the pattern menu copies it to another slot.
  Projects from 1.0.x open with their pattern in slot A and copies of it in
  B-H, and play as before.
- **Export patterns as MIDI clips.** Right-click a slot for **Export MIDI**,
  or use **Export MIDI**, **Export all slots as MIDI** and **Open MIDI
  folder** in the pattern menu. A clip is one pass of the pattern and plays
  as the sequencer does: accents, slides (as overlapping notes), gate and
  swing. On Windows a slot can also be dragged straight into the DAW.
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

[Unreleased]: https://github.com/travisdk/SubForge/compare/v1.2.2...HEAD
[1.2.2]: https://github.com/travisdk/SubForge/compare/v1.2.1...v1.2.2
[1.2.1]: https://github.com/travisdk/SubForge/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/travisdk/SubForge/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/travisdk/SubForge/compare/v1.0.1...v1.1.0
[1.0.1]: https://github.com/travisdk/SubForge/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/travisdk/SubForge/releases/tag/v1.0.0
