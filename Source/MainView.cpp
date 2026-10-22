// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#include "MainView.h"
#include "PresetFiles.h"
#include "SubForgeVersion.h"

namespace
{
using P = SubForgeLookAndFeel::Palette;

// Panel layout at the design size.
const juce::Rectangle<int> kFilterPanel { 16, 64, 520, 254 };
const juce::Rectangle<int> kOscPanel { 546, 64, 358, 122 };
const juce::Rectangle<int> kDrivePanel { 546, 196, 358, 122 };
const juce::Rectangle<int> kAmpPanel { 16, 328, 180, 120 };
const juce::Rectangle<int> kPitchPanel { 204, 328, 136, 120 };
const juce::Rectangle<int> kLfoPanel { 348, 328, 196, 120 };
const juce::Rectangle<int> kModPanel { 552, 328, 116, 120 };
const juce::Rectangle<int> kPlayPanel { 676, 328, 228, 120 };
const juce::Rectangle<int> kSeqPanel { 16, 458, 888, 212 };

constexpr int kPanelTitle = 30; // height reserved for "FILTER" etc.
} // namespace

MainView::MainView (SubForgeProcessor& p, SubForgeLookAndFeel& lf, std::function<void (float)> onScaleChosen,
                    std::function<float()> currentScale, std::function<void()> onDesignHeightChanged)
    : processor (p), lookAndFeel (lf), chooseScale (std::move (onScaleChosen)), getScale (std::move (currentScale)),
      designHeightChanged (std::move (onDesignHeightChanged))
{
    setLookAndFeel (&lookAndFeel); // before any child is laid out, so the controls find the SubForge look

    panels = {
        { "FILTER", kFilterPanel },  { "OSC", kOscPanel },   { "DRIVE", kDrivePanel },
        { "AMP ENV", kAmpPanel },    { "PITCH", kPitchPanel }, { "LFO > CUT", kLfoPanel },
        { "MOD", kModPanel },        { "PLAY", kPlayPanel },   { "SEQUENCER", kSeqPanel, true },
    };

    // header
    presetBox.onChange = [this]
    {
        if (const int id = presetBox.getSelectedId(); id > 0)
            sound().load (id - 1);
    };
    presetPrev.onClick = [this] { stepPreset (-1); };
    presetNext.onClick = [this] { stepPreset (1); };
    saveButton.onClick = [this]
    {
        auto suggestion = sound().name().trimCharactersAtEnd (" *");
        if (! sound().isUser (sound().current()))
            suggestion = "My " + suggestion;
        showNameDialog ("Save preset", sound().folder(), suggestion,
                        [this] (const juce::String& name) { savePreset (name); });
    };
    randomButton.setToggleState (true, juce::dontSendNotification); // drawn filled with the accent
    randomButton.onClick = [this] { sound().randomize(); };
    mutateButton.onClick = [this] { sound().mutate(); };
    settingsButton.onClick = [this] { showSettingsMenu(); };

    for (auto* c : std::initializer_list<juce::Component*> { &presetPrev, &presetBox, &presetNext, &saveButton,
                                                              &randomButton, &mutateButton, &settingsButton })
        addAndMakeVisible (c);

    // sections
    lfoShape.setValueText ([] (double v)
    {
        static const char* names[] = { "SIN", "TRI", "SAW", "SQR", "S&H" };
        return juce::String (names[juce::jlimit (0, 4, juce::roundToInt (v))]);
    });
    lfoSync.setValueText ([] (double v)
    {
        const int i = juce::jlimit (0, subforge::kNumLfoSyncs - 1, juce::roundToInt (v));
        return i == 0 ? juce::String ("FREE") : juce::String (subforge::kLfoSyncNames[i]);
    });
    cutoff.setValueText ([] (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    });
    lfoRetrigAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "lfoRetrig", lfoRetrig);
    seqOnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "seqOn", seqOn);
    seqOn.getProperties().set (SubForgeLookAndFeel::switchProperty, true);

    for (auto* c : std::initializer_list<juce::Component*> {
             &filterMode, &filterCurve, &envAmount, &filterDecay, &drive, &keyTrack, &cutoff, &resonance,
             &wave, &octave, &detune, &sub, &subWave,
             &distType, &distGain, &distTone, &distMix,
             &attack, &decay, &sustain, &release, &pitchEnv, &pitchDecay, &click,
             &lfoSync, &lfoRate, &lfoDepth, &lfoShape, &lfoRetrig,
             &wheelAmount, &aftertouchAmount, &wheelDest, &aftertouchDest,
             &glide, &velocity, &accent, &accentSweep, &volume })
        addAndMakeVisible (c);
    distType.getProperties().set (SubForgeLookAndFeel::secondaryProperty, true);

    // sequencer
    patternBox.onChange = [this]
    {
        if (const int id = patternBox.getSelectedId(); id > 0)
            patterns().load (id - 1);
    };
    patternPrev.onClick = [this] { stepPattern (-1); };
    patternNext.onClick = [this] { stepPattern (1); };
    patternSave.onClick = [this]
    {
        auto suggestion = patterns().name().trimCharactersAtEnd (" *");
        if (! patterns().isUser (patterns().current()))
            suggestion = "My " + suggestion;
        showNameDialog ("Save pattern", patterns().folder(), suggestion,
                        [this] (const juce::String& name) { savePattern (name); });
    };
    generateButton.getProperties().set (SubForgeLookAndFeel::outlineProperty, true);
    generateButton.onClick = [this] { patterns().randomize(); };
    patternMenuButton.onClick = [this] { showPatternMenu(); };

    for (auto* c : std::initializer_list<juce::Component*> { &seqOn, &seqRate, &seqSteps, &seqGate, &seqSwing, &seqRoot,
                                                              &patternPrev, &patternBox, &patternNext, &patternSave,
                                                              &generateButton, &patternMenuButton, &stepGrid, &slotColumn,
                                                              &sequencerToggle })
        addAndMakeVisible (c);
    sequencerToggle.onClick = [this] { setSequencerVisible (! sequencerVisible); };
    sequencerVisible = processor.getSettings().getBoolValue ("showSequencer", true);
    showSequencerDetails (sequencerVisible);

    setTooltips();
    showTooltips (processor.getSettings().getBoolValue ("showTooltips", true));

    processor.addChangeListener (this);
    sound().rescan(); // pick up files added since the plugin was loaded
    patterns().rescan();
    refreshPresetBox();
    refreshPatternBox();
    timerCallback();
    startTimerHz (30);
    setSize (kWidth, designHeight());
}

juce::Rectangle<int> MainView::sequencerPanel() const
{
    return sequencerVisible ? kSeqPanel : kSeqPanel.withHeight (42);
}

void MainView::setTooltips()
{
    // header
    presetPrev.setTooltip ("Previous sound preset.");
    presetNext.setTooltip ("Next sound preset.");
    presetBox.setTooltip ("Choose a sound preset. A sound never changes the pattern or sequencer settings.");
    saveButton.setTooltip ("Save the current sound as a user preset.");
    randomButton.setTooltip ("Create a new random sound. The pattern is left alone.");
    mutateButton.setTooltip ("Nudge the current sound slightly. Click a few times to explore variations.");

    // filter
    filterMode.setTooltip ("Filter type: 24 or 12 dB lowpass, 12 dB bandpass, or diode ladder for a classic acid squelch.");
    filterCurve.setTooltip ("Live view of the filter response. Follows Cutoff, Resonance and mode.");
    envAmount.setTooltip ("How far the envelope opens the filter on each note.");
    filterDecay.setTooltip ("How fast the filter closes after each note. Short values give a plucky bass.");
    drive.setTooltip ("Overdrives the filter input for a thicker, grittier tone.");
    keyTrack.setTooltip ("Lets the cutoff follow the note, so high notes stay as bright as low ones.");
    cutoff.setTooltip ("Filter frequency. The main tone control; automate it for sweeps.");
    resonance.setTooltip ("Emphasis at the cutoff. High values give the squelchy acid sound.");

    // osc
    wave.setTooltip ("Oscillator waveform: saw, square, pulse, sine or triangle.");
    octave.setTooltip ("Shifts the oscillator up or down in octaves.");
    detune.setTooltip ("Detunes a second oscillator for a wider, chorused sound. Keep it low for tight bass.");
    sub.setTooltip ("Level of the sub oscillator one octave below. Adds weight on big speakers.");
    subWave.setTooltip ("Sub oscillator shape: sine for clean weight, square for more presence.");

    // drive
    distType.setTooltip ("Distortion type after the filter. Off bypasses it.");
    distGain.setTooltip ("Amount of distortion. The level is compensated automatically.");
    distTone.setTooltip ("Darker to the left, brighter to the right.");
    distMix.setTooltip ("Blend of dry and distorted signal. Lower it to keep the low end clean.");

    // amp env, pitch
    attack.setTooltip ("Attack: how fast the sound fades in.");
    decay.setTooltip ("Decay: time from peak down to the sustain level.");
    sustain.setTooltip ("Sustain: level held while the note is down.");
    release.setTooltip ("Release: how long the sound rings after the note ends.");
    pitchEnv.setTooltip ("Pitch sweep at the start of each note, in semitones. Negative values drop into the note.");
    pitchDecay.setTooltip ("How fast the pitch sweep returns to the note.");
    click.setTooltip ("Adds a short click at the note start to help the bass cut through.");

    // lfo, mod, play
    lfoSync.setTooltip ("LFO speed locked to the host tempo. Set to FREE to use Rate instead.");
    lfoRate.setTooltip ("LFO speed in Hz when Sync is FREE.");
    lfoDepth.setTooltip ("How much the LFO moves the cutoff.");
    lfoShape.setTooltip ("LFO waveform. S&H gives random steps.");
    lfoRetrig.setTooltip ("Restart the LFO on each new note instead of following the song position.");
    wheelAmount.setTooltip ("How much the mod wheel affects its destination. Negative values invert it.");
    aftertouchAmount.setTooltip ("How much aftertouch affects its destination. Negative values invert it.");
    wheelDest.setTooltip ("What the mod wheel controls.");
    aftertouchDest.setTooltip ("What aftertouch controls.");
    glide.setTooltip ("Time to slide between overlapping notes and sequencer slides.");
    velocity.setTooltip ("How much velocity affects the volume.");
    accent.setTooltip ("Strength of accents (high velocities or accented steps): louder and brighter.");
    accentSweep.setTooltip ("Accents close together build up and open the filter further, like a classic acid line.");
    volume.setTooltip ("Output volume.");

    // sequencer
    sequencerToggle.setTooltip ("Show or hide the step sequencer.");
    seqOn.setTooltip ("Play the pattern while the host is playing. Hold keys to transpose it.");
    seqRate.setTooltip ("Length of each step.");
    seqSteps.setTooltip ("Pattern length. Odd lengths make the pattern drift against the bar.");
    seqGate.setTooltip ("Note length within each step.");
    seqSwing.setTooltip ("Delays every second step for a shuffled groove.");
    seqRoot.setTooltip ("Base note of the pattern. Root note is shared by all slots. Drag or scroll to change it.");
    patternPrev.setTooltip ("Previous pattern.");
    patternNext.setTooltip ("Next pattern.");
    patternBox.setTooltip ("Choose a pattern. Patterns keep the sound unchanged.");
    patternSave.setTooltip ("Save the current pattern as a user pattern.");
    generateButton.setTooltip ("Create a new random pattern.");
    patternMenuButton.setTooltip ("Pattern options: clear, copy to another slot, export as MIDI, copy and paste as text, folder.");
    slotColumn.setTooltip (juce::String ("Pattern slots. Click to choose which one plays; it switches when the pattern starts over. "
                                         "Automate Sequencer Pattern, or play MIDI notes 0-7 (C-2 to G-2 in Bitwig and Live, C0 to G0 in FL Studio) while the sequencer runs. ")
                           + (SlotColumn::canDrag ? "Drag a slot into your DAW, or right-click to export it as MIDI."
                                                  : "Right-click a slot to export it as MIDI."));
    slotColumn.onMenu = [this] (int slot) { showSlotMenu (slot); };
    slotColumn.onExport = [this] (int slot) { return patterns().exportMidi (slot); };
    stepGrid.setTooltips ("Drag or scroll to change the note, click to turn the step on or off, double-click to reset it to the root.",
                          "Accent or slide on this step. Drag across steps to paint.");
}

// Tooltips can be turned off in the settings menu; the choice is remembered globally.
void MainView::showTooltips (bool shown)
{
    if (shown && tooltipWindow == nullptr)
    {
        tooltipWindow = std::make_unique<TooltipWindow> (this, 700);
        tooltipWindow->setOpaque (false); // rounded corners show the view behind
    }
    else if (! shown)
        tooltipWindow.reset();

    processor.getSettings().setValue ("showTooltips", shown);
    processor.getSettings().saveIfNeeded();
}

// Collapsing the sequencer leaves only its title and on/off switch, so a running
// sequencer can still be stopped; the choice is remembered globally.
void MainView::setSequencerVisible (bool visible)
{
    sequencerVisible = visible;
    showSequencerDetails (visible);
    processor.getSettings().setValue ("showSequencer", visible);
    processor.getSettings().saveIfNeeded();
    setSize (kWidth, designHeight());
    resized();
    repaint();
    designHeightChanged();
}

void MainView::showSequencerDetails (bool visible)
{
    for (auto* c : std::initializer_list<juce::Component*> { &seqRate, &seqSteps, &seqGate, &seqSwing, &seqRoot,
                                                              &patternPrev, &patternBox, &patternNext, &patternSave,
                                                              &generateButton, &patternMenuButton, &stepGrid, &slotColumn })
        c->setVisible (visible);
}

MainView::~MainView()
{
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

//==============================================================================
void MainView::paint (juce::Graphics& g)
{
    g.fillAll (P::background);

    // header: SF badge, and beside it the name over the tagline, the two centred on the badge by their ink
    const juce::Rectangle<float> logo { 16.0f, 14.0f, 38.0f, 38.0f };
    drawLogo (g, logo);
    const auto nameFont = lookAndFeel.titleFont (24.0f).withExtraKerningFactor (0.08f);
    const auto taglineFont = lookAndFeel.labelFont (10.5f).withExtraKerningFactor (0.2f);
    const auto inkOf = [] (const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText (font, text, 0.0f, 0.0f);
        return glyphs.getBoundingBox (0, -1, true); // relative to the baseline
    };
    const auto nameInk = inkOf (nameFont, "SUBFORGE"), taglineInk = inkOf (taglineFont, "MONO BASS SYNTH");
    constexpr float lineGap = 6.0f;
    const float top = logo.getCentreY() - (nameInk.getHeight() + lineGap + taglineInk.getHeight()) / 2.0f;
    const float nameBaseline = top - nameInk.getY();
    const float taglineBaseline = top + nameInk.getHeight() + lineGap - taglineInk.getY();
    g.setColour (lookAndFeel.getAccent());
    g.setFont (nameFont);
    g.drawSingleLineText ("SUBFORGE", 64, juce::roundToInt (nameBaseline));
    g.setColour (P::dimText);
    g.setFont (taglineFont);
    g.drawSingleLineText ("MONO BASS SYNTH", 65, juce::roundToInt (taglineBaseline));

    for (const auto& panel : panels)
    {
        const bool sequencer = panel.sequencer;
        const auto r = panel.bounds.toFloat();
        const auto titleArea = panel.bounds.withHeight (kPanelTitle).withTrimmedLeft (14).withTrimmedTop (4);

        // the sequencer zone: darker, accent-edged and titled in the accent colour
        g.setColour (sequencer ? P::background.interpolatedWith (P::panel, 0.4f) : P::panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (sequencer ? lookAndFeel.getAccent().withAlpha (0.35f) : P::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

        if (sequencer)
        {
            // chevron: down when the steps are shown, right when hidden
            const auto c = juce::Point<float> ((float) titleArea.getX() + 5.0f, (float) titleArea.getCentreY());
            juce::Path chevron;
            if (sequencerVisible)
            {
                chevron.startNewSubPath (c.x - 4.0f, c.y - 2.0f);
                chevron.lineTo (c.x, c.y + 2.5f);
                chevron.lineTo (c.x + 4.0f, c.y - 2.0f);
            }
            else
            {
                chevron.startNewSubPath (c.x - 2.0f, c.y - 4.0f);
                chevron.lineTo (c.x + 2.5f, c.y);
                chevron.lineTo (c.x - 2.0f, c.y + 4.0f);
            }
            g.setColour (lookAndFeel.getAccent());
            g.strokePath (chevron, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setFont (lookAndFeel.titleFont (13.0f).withExtraKerningFactor (0.14f));
            g.drawText (panel.title, titleArea.withTrimmedLeft (16), juce::Justification::centredLeft);
            continue;
        }

        g.setColour (P::text.withAlpha (0.85f));
        g.setFont (lookAndFeel.labelFont (13.0f).withExtraKerningFactor (0.2f));
        g.drawText (panel.title, titleArea, juce::Justification::centredLeft);
    }

    // which browser loads what
    g.setColour (P::dimText);
    g.setFont (lookAndFeel.titleFont (10.5f).withExtraKerningFactor (0.18f));
    g.drawText ("SOUND", 334, 16, 46, 34, juce::Justification::centredRight);
    if (sequencerVisible)
    {
        g.setFont (lookAndFeel.titleFont (9.5f).withExtraKerningFactor (0.12f));
        g.drawText ("PATTERN", 520, kSeqPanel.getY() + 10, 40, 24, juce::Justification::centredRight);
    }

    g.setColour (P::dimText);
    g.setFont (lookAndFeel.labelFont (11.0f).withExtraKerningFactor (0.1f));
    g.drawText ("SUB WAVE", 806, 106, 90, 16, juce::Justification::centred);
    g.drawText ("RETRIG", kLfoPanel.getRight() - 72, kLfoPanel.getY() + 6, 46, 24, juce::Justification::centredRight);
}

// The SF badge: a rounded tile in the accent colour (with a slight gradient),
// "SF" cut out in the background colour and a saw-tooth wave running across
// the bottom, all drawn as vectors so it follows the theme and any UI size.
void MainView::drawLogo (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto theme = lookAndFeel.getAccent();
    g.setGradientFill (juce::ColourGradient (theme.brighter (0.15f), r.getX(), r.getY(),
                                             theme.darker (0.35f), r.getRight(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 8.0f);

    g.setColour (P::background);
    g.setFont (lookAndFeel.titleFont (21.0f).withExtraKerningFactor (-0.02f));
    g.drawText ("SF", r.withTrimmedBottom (7.0f).translated (0.0f, 1.0f), juce::Justification::centred);

    // two saw-tooth periods: rise, drop, rise, drop
    const float y0 = r.getBottom() - 6.0f, y1 = r.getBottom() - 11.0f;
    const float x0 = r.getX() + 6.0f, w = r.getWidth() - 12.0f;
    juce::Path saw;
    saw.startNewSubPath (x0, y0);
    saw.lineTo (x0 + w * 0.5f, y1);
    saw.lineTo (x0 + w * 0.5f, y0);
    saw.lineTo (x0 + w, y1);
    saw.lineTo (x0 + w, y0);
    g.strokePath (saw, juce::PathStrokeType (1.6f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
}

// Places knobs (or any components) side by side in equal cells.
void MainView::layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items)
{
    const int n = (int) items.size();
    const float cell = (float) area.getWidth() / (float) n;
    int i = 0;
    for (auto* item : items)
    {
        const int x = area.getX() + juce::roundToInt (cell * (float) i);
        const int w = area.getX() + juce::roundToInt (cell * (float) (i + 1)) - x;
        item->setBounds (x, area.getY(), w, area.getHeight());
        ++i;
    }
}

void MainView::resized()
{
    // header
    presetPrev.setBounds (386, 16, 28, 34);
    presetBox.setBounds (416, 16, 139, 34);
    presetNext.setBounds (557, 16, 28, 34);
    saveButton.setBounds (594, 16, 64, 34);
    randomButton.setBounds (666, 16, 106, 34);
    mutateButton.setBounds (780, 16, 80, 34);
    settingsButton.setBounds (868, 16, 36, 34);

    // filter
    filterMode.setBounds (kFilterPanel.getRight() - 14 - 236, kFilterPanel.getY() + 10, 236, 26);
    filterCurve.setBounds (30, kFilterPanel.getY() + 46, 288, 112);
    layoutKnobRow ({ 30, 226, 288, 84 }, { &envAmount, &filterDecay, &drive, &keyTrack });
    cutoff.setBounds (328, 112, 100, 172);
    resonance.setBounds (430, 112, 100, 172);

    // osc
    wave.setBounds (kOscPanel.getRight() - 14 - 200, kOscPanel.getY() + 10, 200, 24);
    layoutKnobRow ({ 560, 98, 240, 82 }, { &octave, &detune, &sub });
    subWave.setBounds (812, 126, 78, 26);

    // drive
    distType.setBounds (kDrivePanel.getRight() - 14 - 120, kDrivePanel.getY() + 10, 120, 24);
    layoutKnobRow ({ 560, kDrivePanel.getY() + 34, 330, 82 }, { &distGain, &distTone, &distMix });

    // second row
    auto knobArea = [] (juce::Rectangle<int> panel) { return panel.withTrimmedTop (kPanelTitle).reduced (8, 0).withTrimmedBottom (6); };
    layoutKnobRow (knobArea (kAmpPanel), { &attack, &decay, &sustain, &release });
    layoutKnobRow (knobArea (kPitchPanel), { &pitchEnv, &pitchDecay, &click });
    layoutKnobRow (knobArea (kLfoPanel), { &lfoSync, &lfoRate, &lfoDepth, &lfoShape });
    lfoRetrig.setBounds (kLfoPanel.getRight() - 24, kLfoPanel.getY() + 9, 18, 18);
    auto mod = knobArea (kModPanel);
    auto modChips = mod.removeFromBottom (20);
    layoutKnobRow (mod, { &wheelAmount, &aftertouchAmount });
    layoutKnobRow (modChips.reduced (0, 0), { &wheelDest, &aftertouchDest });
    wheelDest.setBounds (wheelDest.getBounds().reduced (2, 0));
    aftertouchDest.setBounds (aftertouchDest.getBounds().reduced (2, 0));
    layoutKnobRow (knobArea (kPlayPanel), { &glide, &velocity, &accent, &accentSweep, &volume });

    // sequencer header (its panel shrinks to this row when the steps are hidden)
    panels.back().bounds = sequencerPanel();
    sequencerToggle.setBounds (kSeqPanel.getX() + 8, kSeqPanel.getY() + 6, 110, 30);
    const int seqTop = kSeqPanel.getY() + 9;
    seqOn.setBounds (136, seqTop + 2, 50, 22);
    int x = 190; // ends clear of the PATTERN label
    for (auto [chip, width] : { std::pair<juce::Component*, int> { &seqRate, 60 }, { &seqSteps, 54 }, { &seqGate, 62 },
                                { &seqSwing, 68 }, { &seqRoot, 60 } })
    {
        chip->setBounds (x, seqTop + 1, width, 24);
        x += width + 3;
    }
    int right = kSeqPanel.getRight() - 14;
    auto placeRight = [&] (juce::Component& c, int width, int gap)
    {
        right -= width;
        c.setBounds (right, seqTop + 1, width, 24);
        right -= gap;
    };
    placeRight (patternMenuButton, 28, 6);
    placeRight (generateButton, 84, 6);
    placeRight (patternSave, 50, 8);
    placeRight (patternNext, 18, 2);
    placeRight (patternBox, 104, 2);
    placeRight (patternPrev, 18, 0);

    slotColumn.setBounds (30, kSeqPanel.getY() + 42, 26, kSeqPanel.getHeight() - 50);
    stepGrid.setBounds (64, kSeqPanel.getY() + 42, 826, kSeqPanel.getHeight() - 50);

    if (aboutOverlay != nullptr)
        aboutOverlay->setBounds (getLocalBounds());
}

//==============================================================================
void MainView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetBox();
    refreshPatternBox();
    stepGrid.repaint(); // the pattern may have come from a preset, state or the generator
    slotColumn.repaint();
}

void MainView::timerCallback()
{
    // Rate only matters while the LFO runs free
    const bool freeRunning = processor.apvts.getRawParameterValue ("lfoSync")->load() < 0.5f;
    lfoRate.setEnabled (freeRunning);
    lfoRate.setAlpha (freeRunning ? 1.0f : 0.35f);

    stepGrid.setPlayhead (processor.getSequencerStep());

    // the slot can change from automation or MIDI; the grid and name follow the one that plays
    auto& bank = processor.getPatternBank();
    const int slot = bank.active(), pendingSlot = bank.pending();
    const bool blinkOn = pendingSlot >= 0 && juce::Time::getMillisecondCounter() % 500 < 250;
    const bool slotChanged = slot != shownSlot;
    if (slotChanged)
    {
        shownSlot = slot;
        refreshPatternBox();
        stepGrid.repaint();
    }
    if (slotChanged || pendingSlot != shownPendingSlot || blinkOn != slotColumn.blinkOn)
    {
        shownPendingSlot = pendingSlot;
        slotColumn.blinkOn = blinkOn;
        slotColumn.repaint();
    }
    const int length = processor.getPattern().groove().length;
    const int root = (int) processor.apvts.getRawParameterValue ("seqRoot")->load();
    if (length != shownSeqLength || root != shownSeqRoot)
    {
        shownSeqLength = length;
        shownSeqRoot = root;
        stepGrid.repaint();
    }
}

// Rebuilt on every change: factory presets, then a "User" section for saved files.
void MainView::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < sound().numEntries(); ++i)
    {
        if (i == subforge::kNumPresets)
            presetBox.addSectionHeading ("User");
        presetBox.addItem (sound().entryName (i), i + 1);
    }

    const auto name = sound().name();
    const int index = sound().current();
    if (name == sound().entryName (index))
        presetBox.setSelectedId (index + 1, juce::dontSendNotification);
    else
        presetBox.setText (name, juce::dontSendNotification);
}

void MainView::stepPreset (int delta)
{
    const int n = sound().numEntries();
    sound().load ((sound().current() + delta + n) % n);
}

void MainView::refreshPatternBox()
{
    patternBox.clear (juce::dontSendNotification);
    patternBox.addSectionHeading ("Factory");
    for (int i = 0; i < patterns().numEntries(); ++i)
    {
        if (i == subforge::kNumFactoryPatterns)
            patternBox.addSectionHeading ("User");
        patternBox.addItem (patterns().entryName (i), i + 1);
    }

    const auto name = patterns().name();
    const int index = patterns().current();
    if (index >= 0 && name == patterns().entryName (index))
        patternBox.setSelectedId (index + 1, juce::dontSendNotification);
    else
        patternBox.setText (name, juce::dontSendNotification);
}

void MainView::stepPattern (int delta)
{
    const int n = patterns().numEntries();
    const int current = patterns().current();
    if (current < 0) // nothing from the list loaded yet: start at either end
        patterns().load (delta > 0 ? 0 : n - 1);
    else
        patterns().load ((current + delta + n) % n);
}

//==============================================================================
void MainView::showSettingsMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&lookAndFeel);
    menu.addSectionHeader ("Accent colour");
    for (const auto& choice : SubForgeLookAndFeel::accentChoices())
    {
        juce::PopupMenu::Item item (choice.name);
        item.colour = choice.colour;
        item.isTicked = choice.colour == lookAndFeel.getAccent();
        item.action = [this, colour = choice.colour] { applyAccent (colour); };
        menu.addItem (item);
    }

    menu.addSectionHeader ("Interface size");
    for (float scale : { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f })
        menu.addItem (juce::String (juce::roundToInt (scale * 100.0f)) + " %", true,
                      std::abs (getScale() - scale) < 0.01f, [this, scale] { chooseScale (scale); });
    menu.addSeparator();
    menu.addItem ("Show tooltips", true, tooltipWindow != nullptr, [this] { showTooltips (tooltipWindow == nullptr); });
    menu.addItem ("MIDI notes 0-7 choose pattern", true, processor.slotNotesEnabled(),
                  [this] { processor.setSlotNotesEnabled (! processor.slotNotesEnabled()); });

    const int entry = sound().current();
    const bool onUserPreset = sound().isUser (entry);
    const bool customFolder = sound().folder() != SoundPresets::defaultFolder();
    menu.addSectionHeader ("Sound presets");
    menu.addItem ("Open preset folder", [this]
    {
        auto folder = sound().folder();
        folder.createDirectory();
        subforge::files::openFolder (folder);
    });
    menu.addItem ("Change preset folder...", [this] { choosePresetFolder(); });
    menu.addItem ("Use default folder", customFolder, false, [this] { changePresetFolder ({}); });
    menu.addItem ("Rescan presets", [this] { sound().rescan(); });
    menu.addItem ("Delete \"" + (onUserPreset ? sound().entryName (entry) : juce::String ("user preset")) + "\"",
                  onUserPreset, false, [this, entry]
                  {
                      confirm ("Delete preset?", "\"" + sound().entryName (entry) + "\" will be moved to the trash.",
                               "Delete", [this, entry] { sound().remove (entry); });
                  });
    menu.addSeparator();
    menu.addItem ("About SubForge...", [this] { showAbout(); });
    menu.addItem ("Version " SUBFORGE_VERSION_STRING, false, false, {}); // exact build, e.g. 1.0.1-2-g945938c-dirty

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton).withMinimumWidth (200)
                            .withDeletionCheck (*this));
}

void MainView::showPatternMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&lookAndFeel);
    menu.addSectionHeader ("Pattern");
    menu.addItem ("Clear pattern", [this] { patterns().clear(); });
    juce::PopupMenu copyMenu;
    const int activeSlot = processor.getPatternBank().active();
    for (int i = 0; i < subforge::kNumPatternSlots; ++i)
        copyMenu.addItem (juce::String::charToString ((juce::juce_wchar) ('A' + i)) + "   " + patterns().slotName (i),
                          i != activeSlot, false, [this, activeSlot, i] { patterns().copySlot (activeSlot, i); });
    menu.addSubMenu ("Copy pattern to slot", copyMenu);
    menu.addSeparator();
    menu.addItem ("Export MIDI", [this, activeSlot] { exportSlots ({ activeSlot }); });
    menu.addItem ("Export all slots as MIDI", [this] { exportSlots ({ 0, 1, 2, 3, 4, 5, 6, 7 }); });
    menu.addItem ("Open MIDI folder", [this] { openMidiFolder(); });
    menu.addSeparator();
    menu.addItem ("Copy pattern as text", [this] { juce::SystemClipboard::copyTextToClipboard (patterns().asText()); });
    menu.addItem ("Paste pattern", [this]
    {
        if (! patterns().fromText (juce::SystemClipboard::getTextFromClipboard()))
            confirm ("Nothing to paste",
                     "The clipboard needs sixteen steps like \"0a 0 12s - 3a ...\" (rest \"-\", a = accent, s = slide).",
                     "OK", {});
    });
    menu.addSeparator();
    menu.addItem ("Open pattern folder", [this]
    {
        auto folder = patterns().folder();
        folder.createDirectory();
        subforge::files::openFolder (folder);
    });
    menu.addItem ("Rescan patterns", [this] { patterns().rescan(); });

    const int entry = patterns().current();
    const bool onUserPattern = patterns().isUser (entry);
    menu.addItem ("Delete \"" + (onUserPattern ? patterns().entryName (entry) : juce::String ("user pattern")) + "\"",
                  onUserPattern, false, [this, entry]
                  {
                      confirm ("Delete pattern?", "\"" + patterns().entryName (entry) + "\" will be moved to the trash.",
                               "Delete", [this, entry] { patterns().remove (entry); });
                  });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&patternMenuButton).withDeletionCheck (*this));
}

void MainView::showSlotMenu (int slot)
{
    const auto letter = juce::String::charToString ((juce::juce_wchar) ('A' + slot));
    juce::PopupMenu menu;
    menu.setLookAndFeel (&lookAndFeel);
    menu.addSectionHeader (letter + " - " + patterns().slotName (slot));
    menu.addItem ("Export MIDI", [this, slot] { exportSlots ({ slot }); });
    juce::PopupMenu copyMenu;
    for (int i = 0; i < subforge::kNumPatternSlots; ++i)
        copyMenu.addItem (juce::String::charToString ((juce::juce_wchar) ('A' + i)) + "   " + patterns().slotName (i),
                          i != slot, false, [this, slot, i] { patterns().copySlot (slot, i); });
    menu.addSubMenu ("Copy to slot", copyMenu);
    // opens beside the slot that was clicked, not below the whole column
    const auto area = slotColumn.localAreaToGlobal (slotColumn.cell (slot).getSmallestIntegerContainer());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (area).withDeletionCheck (*this));
}

// Exports to the MIDI folder and says where, with a button that opens it.
void MainView::exportSlots (std::initializer_list<int> slots)
{
    juce::StringArray written;
    for (int slot : slots)
    {
        const auto file = patterns().exportMidi (slot);
        if (file == juce::File())
        {
            confirm ("Could not export", "Writing to " + patterns().midiFolder().getFullPathName() + " failed.", "OK", {});
            return;
        }
        written.add (file.getFileName());
    }
    const auto what = written.size() == 1 ? written[0] : juce::String (written.size()) + " MIDI files";
    confirm ("MIDI exported", "Saved " + what + " in " + patterns().midiFolder().getFullPathName() + ".",
             "Open folder", [this] { openMidiFolder(); }, "OK");
}

void MainView::openMidiFolder()
{
    auto folder = patterns().midiFolder();
    folder.createDirectory();
    subforge::files::openFolder (folder);
}

void MainView::applyAccent (juce::Colour newAccent)
{
    lookAndFeel.setAccent (newAccent);
    sendLookAndFeelChange();
    repaint();

    processor.getSettings().setValue ("accentColour", newAccent.toString());
    processor.getSettings().saveIfNeeded();
}

//==============================================================================
void MainView::showNameDialog (const juce::String& title, const juce::File& folder, const juce::String& suggestion,
                               std::function<void (const juce::String&)> onName)
{
    auto* window = new juce::AlertWindow (title, "Saved to " + folder.getFullPathName(), juce::MessageBoxIconType::NoIcon);
    window->setLookAndFeel (&lookAndFeel);
    window->addTextEditor ("name", suggestion, "Name:");
    window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    // shown inside the view rather than as a separate desktop window, which some hosts bury
    addAndMakeVisible (window);
    window->setCentrePosition (getLocalBounds().getCentre());
    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [safeThis = juce::Component::SafePointer<MainView> (this), window, onName] (int result)
        {
            const auto name = window->getTextEditorContents ("name").trim();
            if (safeThis != nullptr && result == 1 && name.isNotEmpty())
                onName (name);
        }), true);

    if (auto* nameEditor = window->getTextEditor ("name"))
    {
        nameEditor->grabKeyboardFocus();
        nameEditor->selectAll();
    }
}

void MainView::savePreset (const juce::String& name)
{
    auto save = [this, name]
    {
        if (! sound().save (name))
            confirm ("Could not save", "Writing to " + sound().folder().getFullPathName() + " failed.", "OK", {});
    };

    if (sound().exists (name))
        confirm ("Overwrite preset?", "\"" + name + "\" already exists.", "Overwrite", save);
    else
        save();
}

void MainView::savePattern (const juce::String& name)
{
    auto save = [this, name]
    {
        if (! patterns().save (name))
            confirm ("Could not save", "Writing to " + patterns().folder().getFullPathName() + " failed.", "OK", {});
    };

    if (patterns().exists (name))
        confirm ("Overwrite pattern?", "\"" + name + "\" already exists.", "Overwrite", save);
    else
        save();
}

// Two-button (or one, without onOk) message shown inside the view.
void MainView::confirm (const juce::String& title, const juce::String& message, const juce::String& okText,
                        std::function<void()> onOk, const juce::String& cancelText)
{
    auto* window = new juce::AlertWindow (title, message, juce::MessageBoxIconType::NoIcon);
    window->setLookAndFeel (&lookAndFeel);
    window->addButton (okText, 1, juce::KeyPress (juce::KeyPress::returnKey));
    if (onOk)
        window->addButton (cancelText, 0, juce::KeyPress (juce::KeyPress::escapeKey));

    addAndMakeVisible (window);
    window->setCentrePosition (getLocalBounds().getCentre());
    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [safeThis = juce::Component::SafePointer<MainView> (this), onOk] (int result)
        {
            if (safeThis != nullptr && result == 1 && onOk)
                onOk();
        }), true);
}

// Patterns live in a "Patterns" folder next to the preset folder, so both follow a change.
void MainView::changePresetFolder (const juce::File& folder)
{
    sound().setFolder (folder);
    patterns().rescan();
}

void MainView::choosePresetFolder()
{
    folderChooser = std::make_unique<juce::FileChooser> ("Choose preset folder", sound().folder());
    folderChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                [safeThis = juce::Component::SafePointer<MainView> (this)] (const juce::FileChooser& chooser)
                                {
                                    const auto folder = chooser.getResult();
                                    if (safeThis != nullptr && folder.isDirectory())
                                        safeThis->changePresetFolder (folder);
                                });
}

void MainView::showAbout()
{
    aboutOverlay = std::make_unique<AboutOverlay> (lookAndFeel, [safeThis = juce::Component::SafePointer<MainView> (this)]
    {
        // deferred: the overlay is still inside its own callback when asked to close
        juce::MessageManager::callAsync ([safeThis]
        {
            if (safeThis != nullptr)
                safeThis->aboutOverlay.reset();
        });
    });
    addAndMakeVisible (*aboutOverlay);
    aboutOverlay->setBounds (getLocalBounds());
    aboutOverlay->focusText(); // so Escape works without clicking first (hosts may still keep the keys)
}

void MainView::SettingsButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (P::field);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (isHighlighted ? P::panelEdge.brighter (0.5f) : P::panelEdge.brighter (0.15f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    // gear: eight square teeth around a ring, with a hole in the middle
    const auto centre = bounds.getCentre();
    const auto colour = isDown || isHighlighted ? P::text : P::dimText;
    constexpr int teeth = 8;
    constexpr float outer = 9.5f, root = 7.0f, hole = 3.0f;
    const float step = juce::MathConstants<float>::twoPi / (float) teeth;
    const float halfTooth = 1.6f; // half width of a tooth, so its sides stay parallel

    const float aRoot = std::asin (halfTooth / root), aTip = std::asin (halfTooth / outer);
    const auto at = [centre] (float r, float angle) { return centre.getPointOnCircumference (r, angle); };

    juce::Path gear;
    gear.startNewSubPath (at (root, -aRoot));
    for (int i = 0; i < teeth; ++i)
    {
        const float a = (float) i * step;
        gear.lineTo (at (outer, a - aTip));
        gear.lineTo (at (outer, a + aTip));
        gear.lineTo (at (root, a + aRoot));
        gear.addCentredArc (centre.x, centre.y, root, root, 0.0f, a + aRoot, a + step - aRoot);
    }
    gear.closeSubPath();

    g.setColour (colour);
    g.strokePath (gear, juce::PathStrokeType (1.4f, juce::PathStrokeType::mitered));
    g.drawEllipse (juce::Rectangle<float> (2.0f * hole, 2.0f * hole).withCentre (centre), 1.4f);
}

//==============================================================================
juce::Rectangle<float> MainView::SlotColumn::cell (int slot) const
{
    constexpr float gap = 3.0f;
    const float h = ((float) getHeight() - gap * (float) (subforge::kNumPatternSlots - 1)) / (float) subforge::kNumPatternSlots;
    return { 0.0f, (float) slot * (h + gap), (float) getWidth(), h };
}

void MainView::SlotColumn::paint (juce::Graphics& g)
{
    auto* look = subforge::ui::lookOf (*this);
    if (look == nullptr)
        return;
    const auto accent = look->getAccent();
    const auto& bank = processor.getPatternBank();
    g.setFont (look->titleFont (10.5f));

    for (int i = 0; i < subforge::kNumPatternSlots; ++i)
    {
        const auto r = cell (i).reduced (0.5f);
        const bool active = i == bank.active(), pending = i == bank.pending();
        g.setColour (active ? accent : pending && blinkOn ? accent.withAlpha (0.3f) : P::field.darker (0.15f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (active || pending ? accent : P::panelEdge);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        g.setColour (active ? P::background : pending ? P::text : P::dimText);
        g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + i)), r, juce::Justification::centred);
    }
}

int MainView::SlotColumn::slotAt (juce::Point<float> position) const
{
    for (int i = 0; i < subforge::kNumPatternSlots; ++i)
        if (cell (i).contains (position))
            return i;
    return -1;
}

void MainView::SlotColumn::mouseDown (const juce::MouseEvent& e)
{
    pressedSlot = slotAt (e.position);
    dragStarted = false;
    if (pressedSlot >= 0 && e.mods.isPopupMenu())
    {
        onMenu (pressedSlot);
        pressedSlot = -1;
    }
    else if (! canDrag && pressedSlot >= 0) // nothing to wait for: choose at once
    {
        processor.choosePatternSlot (pressedSlot);
        pressedSlot = -1;
    }
}

// The drag is the operating system's: the host takes the file as it takes one from Explorer.
void MainView::SlotColumn::mouseDrag (const juce::MouseEvent& e)
{
    if (! canDrag || pressedSlot < 0 || dragStarted || e.getDistanceFromDragStart() <= 4)
        return;
    dragStarted = true;
    if (const auto file = onExport (pressedSlot); file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

void MainView::SlotColumn::mouseUp (const juce::MouseEvent& e)
{
    if (pressedSlot >= 0 && ! dragStarted && slotAt (e.position) == pressedSlot)
        processor.choosePatternSlot (pressedSlot);
    pressedSlot = -1;
}
