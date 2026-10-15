// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "PluginProcessor.h"
#include "SubForgeLookAndFeel.h"
#include "Widgets.h"
#include "StepGrid.h"
#include "AboutOverlay.h"

// The whole SubForge interface at its design size (kWidth x kHeight). The
// editor scales this component as a unit, so everything inside it - menus
// opened from it, dialogs and the About panel - follows the chosen UI size.
class MainView : public juce::Component,
                 private juce::ChangeListener,
                 private juce::Timer
{
public:
    static constexpr int kWidth = 920, kHeight = 680;
    static constexpr int kCollapsedHeight = 510; // sequencer steps hidden, its control row kept

    MainView (SubForgeProcessor&, SubForgeLookAndFeel&, std::function<void (float)> onScaleChosen,
              std::function<float()> currentScale, std::function<void()> onDesignHeightChanged);

    // design height depends on whether the sequencer's steps are shown
    int designHeight() const { return sequencerVisible ? kHeight : kCollapsedHeight; }
    ~MainView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = subforge::ui::Knob;

    struct Panel
    {
        juce::String title;
        juce::Rectangle<int> bounds;
        bool sequencer = false;
    };

    // Invisible button over the sequencer title: click to show or hide the steps.
    struct TitleToggle : juce::Button
    {
        TitleToggle() : juce::Button ("Show/hide sequencer") { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
        void paintButton (juce::Graphics&, bool, bool) override {}
    };

    struct SettingsButton : juce::Button
    {
        SettingsButton() : juce::Button ("Settings") { setTooltip ("Settings"); }
        void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
    };

    // Pattern slots A-H beside the steps. The playing slot is filled; a slot that waits
    // for the pattern to start over blinks. Click to choose, right-click for the slot's
    // menu. On Windows a slot can also be dragged into the host as a MIDI clip; Linux
    // hosts don't reliably take a file dragged from a plug-in (JUCE's XDND source).
    struct SlotColumn : juce::Component, juce::SettableTooltipClient
    {
       #if JUCE_WINDOWS
        static constexpr bool canDrag = true;
       #else
        static constexpr bool canDrag = false;
       #endif

        explicit SlotColumn (SubForgeProcessor& p) : processor (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        juce::Rectangle<float> cell (int slot) const;
        int slotAt (juce::Point<float>) const;

        SubForgeProcessor& processor;
        std::function<void (int)> onMenu;
        std::function<juce::File (int)> onExport;
        bool blinkOn = false;
        int pressedSlot = -1;
        bool dragStarted = false;
    };

    // JUCE only shows tips while the plug-in's window has keyboard focus on Linux, and
    // hosts can take the focus away (e.g. after loading a pattern), which stopped all
    // tips. The window lives inside the view, so the mouse being over it is enough.
    struct TooltipWindow : juce::TooltipWindow
    {
        using juce::TooltipWindow::TooltipWindow;
        juce::String getTipFor (juce::Component& c) override
        {
            if (auto* client = dynamic_cast<juce::TooltipClient*> (&c);
                client != nullptr && ! juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown()
                && ! c.isCurrentlyBlockedByAnotherModalComponent())
                return client->getTooltip();
            return {};
        }
    };

    juce::RangedAudioParameter& param (const char* id) { return *processor.apvts.getParameter (id); }
    SoundPresets& sound() { return processor.getSoundPresets(); }
    PatternPresets& patterns() { return processor.getPatternPresets(); }
    Knob::Options small (bool secondary = false) const { return { 34, secondary, 14.0f, 12.5f }; }

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void refreshPresetBox();
    void stepPreset (int delta);
    void refreshPatternBox();
    void stepPattern (int delta);
    void showSettingsMenu();
    void setTooltips();
    void showTooltips (bool shown);
    void showPatternMenu();
    void showSlotMenu (int slot);
    void exportSlots (std::initializer_list<int> slots);
    void openMidiFolder();
    void applyAccent (juce::Colour accent);
    void showNameDialog (const juce::String& title, const juce::File& folder, const juce::String& suggestion,
                         std::function<void (const juce::String&)> onName);
    void savePreset (const juce::String& name);
    void savePattern (const juce::String& name);
    void confirm (const juce::String& title, const juce::String& message, const juce::String& okText,
                  std::function<void()> onOk, const juce::String& cancelText = "Cancel");
    void choosePresetFolder();
    void changePresetFolder (const juce::File& folder);
    void showAbout();
    void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items);
    void drawLogo (juce::Graphics&, juce::Rectangle<float> area);
    void setSequencerVisible (bool visible);
    void showSequencerDetails (bool visible);
    juce::Rectangle<int> sequencerPanel() const;

    SubForgeProcessor& processor;
    SubForgeLookAndFeel& lookAndFeel;
    std::function<void (float)> chooseScale;
    std::function<float()> getScale;
    std::function<void()> designHeightChanged;
    bool sequencerVisible = true;
    TitleToggle sequencerToggle;

    std::vector<Panel> panels;

    // header
    juce::ComboBox presetBox;
    juce::TextButton presetPrev { "<" }, presetNext { ">" }, saveButton { "SAVE" };
    juce::TextButton randomButton { "RANDOMIZE" }, mutateButton { "MUTATE" };
    SettingsButton settingsButton;
    std::unique_ptr<TooltipWindow> tooltipWindow; // child of the view, so tooltips scale with it; null when off

    // filter
    subforge::ui::SegmentedChoice filterMode { param ("filterMode"), { "LP24", "LP12", "BP12", "DIODE" } };
    subforge::ui::FilterCurve filterCurve { processor.apvts };
    Knob envAmount { processor.apvts, "envAmount", "Env", small() };
    Knob filterDecay { processor.apvts, "filterDecay", "Decay", small() };
    Knob drive { processor.apvts, "drive", "Drive", small() };
    Knob keyTrack { processor.apvts, "keyTrack", "Key", small() };
    Knob cutoff { processor.apvts, "cutoff", "Cutoff", { 92, false, 26.0f, 14.0f } };
    Knob resonance { processor.apvts, "resonance", "Resonance", { 92, false, 26.0f, 14.0f } };

    // osc
    subforge::ui::SegmentedChoice wave { param ("wave"), { "SAW", "SQR", "PLS", "SIN", "TRI" } };
    Knob octave { processor.apvts, "octave", "Octave", small() };
    Knob detune { processor.apvts, "detune", "Detune", small() };
    Knob sub { processor.apvts, "sub", "Sub", small() };
    subforge::ui::ChoiceChip subWave { param ("subWave"), { "Sine", "Square" } };

    // drive
    subforge::ui::ChoiceChip distType { param ("distType") };
    Knob distGain { processor.apvts, "distDrive", "Gain", small (true) };
    Knob distTone { processor.apvts, "distTone", "Tone", small (true) };
    Knob distMix { processor.apvts, "distMix", "Mix", small (true) };

    // amp env, pitch
    Knob attack { processor.apvts, "attack", "A", small() };
    Knob decay { processor.apvts, "decay", "D", small() };
    Knob sustain { processor.apvts, "sustain", "S", small() };
    Knob release { processor.apvts, "release", "R", small() };
    Knob pitchEnv { processor.apvts, "pitchEnv", "Env", small() };
    Knob pitchDecay { processor.apvts, "pitchDecay", "Decay", small() };
    Knob click { processor.apvts, "click", "Click", small() };

    // lfo, mod, play
    Knob lfoSync { processor.apvts, "lfoSync", "Sync", small() };
    Knob lfoRate { processor.apvts, "lfoRate", "Rate", small() };
    Knob lfoDepth { processor.apvts, "lfoDepth", "Depth", small() };
    Knob lfoShape { processor.apvts, "lfoShape", "Shape", small() };
    juce::ToggleButton lfoRetrig;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfoRetrigAttachment;
    Knob wheelAmount { processor.apvts, "wheelAmount", "Wheel", small() };
    Knob aftertouchAmount { processor.apvts, "aftertouchAmount", "Aftr", small() };
    subforge::ui::ChoiceChip wheelDest { param ("wheelDest") };
    subforge::ui::ChoiceChip aftertouchDest { param ("aftertouchDest") };
    Knob glide { processor.apvts, "glide", "Glide", small() };
    Knob velocity { processor.apvts, "velocity", "Vel", small() };
    Knob accent { processor.apvts, "accent", "Acc", small() };
    Knob accentSweep { processor.apvts, "accentSweep", "Sweep", small() };
    Knob volume { processor.apvts, "volume", "Vol", small() };

    // sequencer
    juce::ToggleButton seqOn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqOnAttachment;
    subforge::ui::ValueChip seqRate { param ("seqRate"), "Rate" };
    subforge::ui::ValueChip seqSteps { param ("seqLength"), "Steps" };
    subforge::ui::ValueChip seqGate { param ("seqGate"), "Gate" };
    subforge::ui::ValueChip seqSwing { param ("seqSwing"), "Swing" };
    subforge::ui::ValueChip seqRoot { param ("seqRoot"), "Root" };
    juce::ComboBox patternBox;
    juce::TextButton patternPrev { "<" }, patternNext { ">" }, patternSave { "SAVE" };
    juce::TextButton generateButton { "GENERATE" }, patternMenuButton { "..." };
    StepGrid stepGrid { processor, lookAndFeel };
    SlotColumn slotColumn { processor };
    int shownSeqLength = -1, shownSeqRoot = -1, shownSlot = -1, shownPendingSlot = -1;

    std::unique_ptr<juce::FileChooser> folderChooser;
    std::unique_ptr<AboutOverlay> aboutOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
