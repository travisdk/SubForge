#include "PluginEditor.h"

namespace
{
using P = SubForgeLookAndFeel::Palette;

constexpr int kHeaderHeight = 48;
constexpr int kMargin = 10;
constexpr int kRowHeight = 128;
constexpr int kSectionTitleHeight = 22;
constexpr int kLabelHeight = 16;
constexpr int kValueHeight = 16;
constexpr int kKnobSize = SubForgeLookAndFeel::kKnobSize;

} // namespace

SubForgeEditor::SubForgeEditor (SubForgeProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    lookAndFeel.setAccent (juce::Colour::fromString (
        processor.getSettings().getValue ("accentColour", SubForgeLookAndFeel::accentChoices().front().colour.toString())));

    sections = {
        { "OSCILLATOR", { choice ("wave", "Wave"), knob ("octave", "Octave"), knob ("detune", "Detune"),
                          knob ("sub", "Sub"), choice ("subWave", "Sub Wave") } },
        { "FILTER", { choice ("filterMode", "Mode"), knob ("cutoff", "Cutoff"), knob ("resonance", "Resonance"),
                      knob ("envAmount", "Env Amt"), knob ("filterDecay", "F. Decay"), knob ("drive", "Drive"),
                      knob ("keyTrack", "Key Track") } },
        { "AMP ENV", { knob ("attack", "Attack"), knob ("decay", "Decay"), knob ("sustain", "Sustain"),
                       knob ("release", "Release") } },
        { "PITCH & CLICK", { knob ("pitchEnv", "Pitch Env"), knob ("pitchDecay", "P. Decay"), knob ("click", "Click") } },
        { "DISTORTION", { choice ("distType", "Type"), knob ("distDrive", "Gain"), knob ("distTone", "Tone"),
                          knob ("distMix", "Mix") } },
        { "LFO > CUTOFF", { choice ("lfoShape", "Shape"), choice ("lfoSync", "Sync"),
                            lfoRateControl = knob ("lfoRate", "Rate"), knob ("lfoDepth", "Depth"),
                            toggle ("lfoRetrig", "Retrig") } },
        { "PLAY", { knob ("glide", "Glide"), knob ("velocity", "Velocity"), knob ("accent", "Accent"),
                    knob ("volume", "Volume") } },
    };

    presetBox.onChange = [this]
    {
        if (const int id = presetBox.getSelectedId(); id > 0)
            processor.loadEntry (id - 1);
    };
    saveButton.onClick = [this] { showSaveDialog(); };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    randomButton.onClick = [this] { processor.randomizePatch(); };
    mutateButton.onClick = [this] { processor.mutatePatch(); };
    randomButton.setColour (juce::TextButton::buttonColourId, lookAndFeel.getAccent());
    randomButton.setColour (juce::TextButton::textColourOffId, P::background);
    settingsButton.onClick = [this] { showSettingsMenu(); };

    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &prevButton, &nextButton, &saveButton,
                                                              &randomButton, &mutateButton, &settingsButton })
        addAndMakeVisible (c);

    processor.addChangeListener (this);
    processor.rescanUserPresets(); // pick up files added since the plugin was loaded
    refreshPresetBox();
    timerCallback();
    startTimerHz (10);
    setSize (900, kHeaderHeight + 3 * kRowHeight + 3 * kMargin);
}

SubForgeEditor::~SubForgeEditor()
{
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

SubForgeEditor::Control* SubForgeEditor::addControl (Kind kind, const char* paramId, const juce::String& labelText)
{
    using APVTS = juce::AudioProcessorValueTreeState;
    auto control = std::make_unique<Control>();
    control->kind = kind;

    switch (kind)
    {
        case Kind::Knob:
        {
            control->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            control->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, kValueHeight);
            // set on the slider itself: its text box is created before our look-and-feel applies
            control->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            control->sliderAttachment = std::make_unique<APVTS::SliderAttachment> (processor.apvts, paramId, control->slider);
            auto* param = processor.apvts.getParameter (paramId);
            control->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
            addAndMakeVisible (control->slider);
            break;
        }
        case Kind::Choice:
            if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
                control->box.addItemList (param->choices, 1);
            control->boxAttachment = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, paramId, control->box);
            addAndMakeVisible (control->box);
            break;
        case Kind::Toggle:
            control->toggleAttachment = std::make_unique<APVTS::ButtonAttachment> (processor.apvts, paramId, control->toggle);
            addAndMakeVisible (control->toggle);
            break;
    }

    control->label.setText (labelText, juce::dontSendNotification);
    control->label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (control->label);
    return controls.emplace_back (std::move (control)).get();
}

void SubForgeEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetBox();
}

void SubForgeEditor::timerCallback()
{
    // Rate only matters while the LFO runs free
    const bool freeRunning = processor.apvts.getRawParameterValue ("lfoSync")->load() < 0.5f;
    lfoRateControl->slider.setEnabled (freeRunning);
    lfoRateControl->slider.setAlpha (freeRunning ? 1.0f : 0.35f);
}

// Rebuilt on every change: factory presets, then a "User" section for saved files.
void SubForgeEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < processor.getNumEntries(); ++i)
    {
        if (i == subforge::kNumPresets)
            presetBox.addSectionHeading ("User");
        presetBox.addItem (processor.getEntryName (i), i + 1);
    }

    const auto name = processor.getPatchName();
    const int index = processor.getCurrentEntry();

    if (name == processor.getEntryName (index))
        presetBox.setSelectedId (index + 1, juce::dontSendNotification);
    else
        presetBox.setText (name, juce::dontSendNotification);
}

void SubForgeEditor::stepPreset (int delta)
{
    const int n = processor.getNumEntries();
    processor.loadEntry ((processor.getCurrentEntry() + delta + n) % n);
}

void SubForgeEditor::showSaveDialog()
{
    auto suggestion = processor.getPatchName().trimCharactersAtEnd (" *");
    if (! processor.isUserEntry (processor.getCurrentEntry()))
        suggestion = "My " + suggestion;

    auto* window = new juce::AlertWindow ("Save preset", "Saved to " + processor.getPresetFolder().getFullPathName(),
                                          juce::MessageBoxIconType::NoIcon);
    window->setLookAndFeel (&lookAndFeel);
    window->addTextEditor ("name", suggestion, "Name:");
    window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    // shown inside the editor rather than as a separate desktop window, which some hosts bury
    addAndMakeVisible (window);
    window->setCentrePosition (getLocalBounds().getCentre());
    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [safeThis = juce::Component::SafePointer<SubForgeEditor> (this), window] (int result)
        {
            const auto name = window->getTextEditorContents ("name").trim();
            if (safeThis != nullptr && result == 1 && name.isNotEmpty())
                safeThis->savePreset (name);
        }), true);

    if (auto* nameEditor = window->getTextEditor ("name"))
    {
        nameEditor->grabKeyboardFocus();
        nameEditor->selectAll();
    }
}

void SubForgeEditor::savePreset (const juce::String& name)
{
    auto save = [this, name]
    {
        if (! processor.saveUserPreset (name))
            confirm ("Could not save", "Writing to " + processor.getPresetFolder().getFullPathName() + " failed.", "OK", {});
    };

    if (processor.userPresetExists (name))
        confirm ("Overwrite preset?", "\"" + name + "\" already exists.", "Overwrite", save);
    else
        save();
}

// Two-button (or one, without onOk) message shown inside the editor.
void SubForgeEditor::confirm (const juce::String& title, const juce::String& message, const juce::String& okText,
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
        [safeThis = juce::Component::SafePointer<SubForgeEditor> (this), onOk] (int result)
        {
            if (safeThis != nullptr && result == 1 && onOk)
                onOk();
        }), true);
}

void SubForgeEditor::choosePresetFolder()
{
    folderChooser = std::make_unique<juce::FileChooser> ("Choose preset folder", processor.getPresetFolder());
    folderChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                [safeThis = juce::Component::SafePointer<SubForgeEditor> (this)] (const juce::FileChooser& chooser)
                                {
                                    const auto folder = chooser.getResult();
                                    if (safeThis != nullptr && folder.isDirectory())
                                        safeThis->processor.setPresetFolder (folder);
                                });
}

void SubForgeEditor::showSettingsMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&lookAndFeel);
    menu.addSectionHeader ("Accent colour");

    const auto& choices = SubForgeLookAndFeel::accentChoices();
    for (size_t i = 0; i < choices.size(); ++i)
    {
        juce::PopupMenu::Item item (choices[i].name);
        item.colour = choices[i].colour;
        item.isTicked = choices[i].colour == lookAndFeel.getAccent();
        item.action = [this, colour = choices[i].colour] { applyAccent (colour); };
        menu.addItem (item);
    }

    const int entry = processor.getCurrentEntry();
    const bool onUserPreset = processor.isUserEntry (entry);
    const bool customFolder = processor.getPresetFolder() != SubForgeProcessor::defaultPresetFolder();
    menu.addSectionHeader ("Presets");
    menu.addItem ("Open preset folder", [this]
    {
        auto folder = processor.getPresetFolder();
        folder.createDirectory();
        folder.startAsProcess();
    });
    menu.addItem ("Change preset folder...", [this] { choosePresetFolder(); });
    menu.addItem ("Use default folder", customFolder, false, [this] { processor.setPresetFolder ({}); });
    menu.addItem ("Rescan presets", [this] { processor.rescanUserPresets(); });
    menu.addItem ("Delete \"" + (onUserPreset ? processor.getEntryName (entry) : juce::String ("user preset")) + "\"",
                  onUserPreset, false, [this, entry]
                  {
                      confirm ("Delete preset?", "\"" + processor.getEntryName (entry) + "\" will be moved to the trash.",
                               "Delete", [this, entry] { processor.deleteUserPreset (entry); });
                  });

    // open below the gear, right-aligned with it so it stays inside the editor
    constexpr int menuWidth = 200;
    const auto gear = settingsButton.getScreenBounds();
    const auto target = juce::Rectangle<int> (gear.getRight() - menuWidth, gear.getY(), menuWidth, gear.getHeight());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (target).withMinimumWidth (menuWidth)
                            .withDeletionCheck (*this));
}

void SubForgeEditor::applyAccent (juce::Colour accent)
{
    lookAndFeel.setAccent (accent);
    randomButton.setColour (juce::TextButton::buttonColourId, accent);
    sendLookAndFeelChange();
    repaint();

    processor.getSettings().setValue ("accentColour", accent.toString());
    processor.getSettings().saveIfNeeded();
}

void SubForgeEditor::SettingsButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    const float outer = 9.0f, toothLength = 3.0f, toothWidth = 3.6f;

    juce::Path gear;
    gear.addEllipse (juce::Rectangle<float> (2.0f * (outer - toothLength + 0.5f), 2.0f * (outer - toothLength + 0.5f))
                         .withCentre (centre));
    for (int i = 0; i < 8; ++i)
    {
        juce::Path tooth;
        tooth.addRoundedRectangle (centre.x - toothWidth * 0.5f, centre.y - outer, toothWidth, toothLength + 1.0f, 1.0f);
        tooth.applyTransform (juce::AffineTransform::rotation ((float) i * juce::MathConstants<float>::pi / 4.0f,
                                                               centre.x, centre.y));
        gear.addPath (tooth);
    }

    g.setColour (isDown || isHighlighted ? P::text : P::dimText);
    g.fillPath (gear);
    g.setColour (P::background);
    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (centre));
}

void SubForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (P::background);

    auto title = headerArea.withTrimmedLeft (kMargin + 2);
    g.setColour (lookAndFeel.getAccent());
    g.setFont (lookAndFeel.boldFont (20.0f).withExtraKerningFactor (0.08f));
    g.drawText ("SUBFORGE", title, juce::Justification::centredLeft);
    g.setColour (P::dimText);
    g.setFont (lookAndFeel.regularFont (11.0f).withExtraKerningFactor (0.1f));
    g.drawText ("MONO BASS SYNTH", title.withTrimmedLeft (122), juce::Justification::centredLeft);

    for (const auto& section : sections)
    {
        const auto bounds = section.bounds.toFloat();
        g.setColour (P::panel);
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (P::panelEdge);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

        g.setColour (lookAndFeel.getAccent());
        g.fillRoundedRectangle (bounds.getX() + 10.0f, bounds.getY() + 8.0f, 3.0f, 9.0f, 1.5f);
        g.setColour (P::dimText);
        g.setFont (lookAndFeel.boldFont (10.5f).withExtraKerningFactor (0.12f));
        g.drawText (section.title, section.bounds.withHeight (kSectionTitleHeight + 3).withTrimmedLeft (19),
                    juce::Justification::centredLeft);
    }
}

void SubForgeEditor::layoutSection (Section& section, juce::Rectangle<int> area)
{
    section.bounds = area;
    auto content = area.reduced (6, 4).withTrimmedTop (kSectionTitleHeight);
    const int contentWidth = content.getWidth();
    const int totalWeight = section.weight();

    for (auto* control : section.controls)
    {
        auto cell = content.removeFromLeft (contentWidth * control->weight() / totalWeight).reduced (3, 0);
        control->label.setBounds (cell.removeFromTop (kLabelHeight));
        const auto knobArea = cell.removeFromTop (kKnobSize + kValueHeight);
        const int knobCentreY = knobArea.getY() + kKnobSize / 2;

        switch (control->kind)
        {
            case Kind::Knob:
                control->slider.setBounds (knobArea);
                break;
            case Kind::Choice:
                control->box.setBounds (cell.getX(), knobCentreY - 12, cell.getWidth(), 24);
                break;
            case Kind::Toggle:
                control->toggle.setBounds (juce::Rectangle<int> (28, 28).withCentre ({ cell.getCentreX(), knobCentreY }));
                break;
        }
    }
}

// Sections in a row share its width in proportion to their controls' weights.
void SubForgeEditor::layoutRow (std::initializer_list<Section*> row, juce::Rectangle<int> area)
{
    int totalWeight = 0;
    for (auto* section : row)
        totalWeight += section->weight();

    const int available = area.getWidth() - kMargin * ((int) row.size() - 1);
    int index = 0;
    for (auto* section : row)
    {
        const bool last = ++index == (int) row.size();
        const int width = last ? area.getWidth() : available * section->weight() / totalWeight;
        layoutSection (*section, area.removeFromLeft (width));
        area.removeFromLeft (kMargin);
    }
}

void SubForgeEditor::resized()
{
    auto area = getLocalBounds();
    headerArea = area.removeFromTop (kHeaderHeight);

    auto header = headerArea.reduced (kMargin, 10);
    settingsButton.setBounds (header.removeFromRight (28));
    header.removeFromRight (8);
    mutateButton.setBounds (header.removeFromRight (76));
    header.removeFromRight (6);
    randomButton.setBounds (header.removeFromRight (96));
    header.removeFromRight (14);
    saveButton.setBounds (header.removeFromRight (56));
    header.removeFromRight (6);
    nextButton.setBounds (header.removeFromRight (28));
    header.removeFromRight (4);
    presetBox.setBounds (header.removeFromRight (210));
    header.removeFromRight (4);
    prevButton.setBounds (header.removeFromRight (28));

    area.reduce (kMargin, 0);

    auto nextRow = [&]
    {
        auto row = area.removeFromTop (kRowHeight);
        area.removeFromTop (kMargin);
        return row;
    };

    layoutRow ({ &sections[0], &sections[1] }, nextRow());
    layoutRow ({ &sections[2], &sections[3], &sections[4] }, nextRow());
    layoutRow ({ &sections[5], &sections[6] }, nextRow());
}
