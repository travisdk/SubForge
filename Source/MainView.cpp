#include "MainView.h"

namespace
{
using P = SubForgeLookAndFeel::Palette;

// Panel layout at the design size, following docs/design/syre.png.
const juce::Rectangle<int> kFilterPanel { 16, 64, 520, 254 };
const juce::Rectangle<int> kOscPanel { 546, 64, 358, 122 };
const juce::Rectangle<int> kDrivePanel { 546, 196, 358, 122 };
const juce::Rectangle<int> kAmpPanel { 16, 328, 180, 120 };
const juce::Rectangle<int> kPitchPanel { 204, 328, 136, 120 };
const juce::Rectangle<int> kLfoPanel { 348, 328, 196, 120 };
const juce::Rectangle<int> kModPanel { 552, 328, 116, 120 };
const juce::Rectangle<int> kPlayPanel { 676, 328, 228, 120 };
const juce::Rectangle<int> kSeqPanel { 16, 458, 888, 212 };

constexpr int kPanelTitle = 30; // height reserved for "02 FILTER" etc.
} // namespace

MainView::MainView (SubForgeProcessor& p, SubForgeLookAndFeel& lf, std::function<void (float)> onScaleChosen,
                    std::function<float()> currentScale)
    : processor (p), lookAndFeel (lf), chooseScale (std::move (onScaleChosen)), getScale (std::move (currentScale))
{
    setLookAndFeel (&lookAndFeel); // before any child is laid out, so the controls find the SubForge look

    panels = {
        { "02", "FILTER", kFilterPanel }, { "01", "OSC", kOscPanel }, { "03", "DRIVE", kDrivePanel },
        { "04", "AMP ENV", kAmpPanel },   { "05", "PITCH", kPitchPanel }, { "06", "LFO > CUT", kLfoPanel },
        { "07", "MOD", kModPanel },       { "09", "PLAY", kPlayPanel },   { "08", "SEQUENCER", kSeqPanel },
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
                                                              &generateButton, &patternMenuButton, &stepGrid })
        addAndMakeVisible (c);

    processor.addChangeListener (this);
    sound().rescan(); // pick up files added since the plugin was loaded
    patterns().rescan();
    refreshPresetBox();
    refreshPatternBox();
    timerCallback();
    startTimerHz (30);
    setSize (kWidth, kHeight);
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

    // header
    g.setColour (lookAndFeel.getAccent());
    g.setFont (lookAndFeel.titleFont (32.0f).withExtraKerningFactor (0.06f));
    g.drawText ("SUBFORGE", 16, 10, 160, 44, juce::Justification::centredLeft);
    g.setColour (P::dimText);
    g.setFont (lookAndFeel.labelFont (11.0f).withExtraKerningFactor (0.22f));
    g.drawText ("MONO BASS SYNTH", 178, 24, 140, 20, juce::Justification::centredLeft);

    for (const auto& panel : panels)
    {
        const auto r = panel.bounds.toFloat();
        g.setColour (P::panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (P::panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

        const auto titleArea = panel.bounds.withHeight (kPanelTitle).withTrimmedLeft (14).withTrimmedTop (4);
        g.setColour (lookAndFeel.getAccent());
        g.setFont (lookAndFeel.valueFont (13.0f));
        g.drawText (panel.number, titleArea, juce::Justification::centredLeft);
        g.setColour (P::text.withAlpha (0.85f));
        g.setFont (lookAndFeel.labelFont (13.0f).withExtraKerningFactor (0.2f));
        g.drawText (panel.title, titleArea.withTrimmedLeft (26), juce::Justification::centredLeft);
    }

    g.setColour (P::dimText);
    g.setFont (lookAndFeel.labelFont (11.0f).withExtraKerningFactor (0.1f));
    g.drawText ("SUB WAVE", 806, 106, 90, 16, juce::Justification::centred);
    g.drawText ("RETRIG", kLfoPanel.getRight() - 72, kLfoPanel.getY() + 6, 46, 24, juce::Justification::centredRight);
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
    presetPrev.setBounds (327, 16, 28, 34);
    presetBox.setBounds (357, 16, 198, 34);
    presetNext.setBounds (557, 16, 28, 34);
    saveButton.setBounds (594, 16, 64, 34);
    randomButton.setBounds (666, 16, 106, 34);
    mutateButton.setBounds (780, 16, 80, 34);
    settingsButton.setBounds (868, 16, 36, 34);

    // 02 filter
    filterMode.setBounds (kFilterPanel.getRight() - 14 - 236, kFilterPanel.getY() + 10, 236, 26);
    filterCurve.setBounds (30, kFilterPanel.getY() + 46, 288, 112);
    layoutKnobRow ({ 30, 226, 288, 84 }, { &envAmount, &filterDecay, &drive, &keyTrack });
    cutoff.setBounds (328, 112, 100, 172);
    resonance.setBounds (430, 112, 100, 172);

    // 01 osc
    wave.setBounds (kOscPanel.getRight() - 14 - 200, kOscPanel.getY() + 10, 200, 24);
    layoutKnobRow ({ 560, 98, 240, 82 }, { &octave, &detune, &sub });
    subWave.setBounds (812, 126, 78, 26);

    // 03 drive
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

    // 08 sequencer header
    const int seqTop = kSeqPanel.getY() + 9;
    seqOn.setBounds (156, seqTop, 26, 26);
    int x = 190;
    for (auto [chip, width] : { std::pair<juce::Component*, int> { &seqRate, 64 }, { &seqSteps, 58 }, { &seqGate, 68 },
                                { &seqSwing, 74 }, { &seqRoot, 70 } })
    {
        chip->setBounds (x, seqTop + 1, width, 24);
        x += width + 4;
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
    placeRight (patternBox, 112, 2);
    placeRight (patternPrev, 18, 0);

    stepGrid.setBounds (30, kSeqPanel.getY() + 42, 860, kSeqPanel.getHeight() - 50);

    if (aboutOverlay != nullptr)
        aboutOverlay->setBounds (getLocalBounds());
}

//==============================================================================
void MainView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetBox();
    refreshPatternBox();
    stepGrid.repaint(); // the pattern may have come from a preset, state or the generator
}

void MainView::timerCallback()
{
    // Rate only matters while the LFO runs free
    const bool freeRunning = processor.apvts.getRawParameterValue ("lfoSync")->load() < 0.5f;
    lfoRate.setEnabled (freeRunning);
    lfoRate.setAlpha (freeRunning ? 1.0f : 0.35f);

    stepGrid.setPlayhead (processor.getSequencerStep());
    const int length = (int) processor.apvts.getRawParameterValue ("seqLength")->load();
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

    const int entry = sound().current();
    const bool onUserPreset = sound().isUser (entry);
    const bool customFolder = sound().folder() != SoundPresets::defaultFolder();
    menu.addSectionHeader ("Presets");
    menu.addItem ("Open preset folder", [this]
    {
        auto folder = sound().folder();
        folder.createDirectory();
        folder.startAsProcess();
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

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton).withMinimumWidth (200)
                            .withDeletionCheck (*this));
}

void MainView::showPatternMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&lookAndFeel);
    menu.addItem ("Clear pattern", [this] { patterns().clear(); });
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
        folder.startAsProcess();
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
                        std::function<void()> onOk)
{
    auto* window = new juce::AlertWindow (title, message, juce::MessageBoxIconType::NoIcon);
    window->setLookAndFeel (&lookAndFeel);
    window->addButton (okText, 1, juce::KeyPress (juce::KeyPress::returnKey));
    if (onOk)
        window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

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
    aboutOverlay = std::make_unique<AboutOverlay> (lookAndFeel, [this]
    {
        // deferred: the overlay is still inside its own callback when asked to close
        juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<MainView> (this)]
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

    // sun-like gear: ring plus eight short rays
    const auto centre = bounds.getCentre();
    const auto colour = isDown || isHighlighted ? P::text : P::dimText;
    g.setColour (colour);
    g.drawEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (centre), 1.6f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::pi / 4.0f;
        g.drawLine ({ centre.getPointOnCircumference (7.0f, a), centre.getPointOnCircumference (9.5f, a) }, 1.6f);
    }
}
