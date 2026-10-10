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

    MainView (SubForgeProcessor&, SubForgeLookAndFeel&, std::function<void (float)> onScaleChosen,
              std::function<float()> currentScale);
    ~MainView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Knob = subforge::ui::Knob;

    struct Panel
    {
        juce::String number, title;
        juce::Rectangle<int> bounds;
    };

    struct SettingsButton : juce::Button
    {
        SettingsButton() : juce::Button ("Settings") {}
        void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
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
    void showPatternMenu();
    void applyAccent (juce::Colour accent);
    void showNameDialog (const juce::String& title, const juce::File& folder, const juce::String& suggestion,
                         std::function<void (const juce::String&)> onName);
    void savePreset (const juce::String& name);
    void savePattern (const juce::String& name);
    void confirm (const juce::String& title, const juce::String& message, const juce::String& okText,
                  std::function<void()> onOk);
    void choosePresetFolder();
    void changePresetFolder (const juce::File& folder);
    void showAbout();
    void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items);

    SubForgeProcessor& processor;
    SubForgeLookAndFeel& lookAndFeel;
    std::function<void (float)> chooseScale;
    std::function<float()> getScale;

    std::vector<Panel> panels;

    // header
    juce::ComboBox presetBox;
    juce::TextButton presetPrev { "<" }, presetNext { ">" }, saveButton { "SAVE" };
    juce::TextButton randomButton { "RANDOMIZE" }, mutateButton { "MUTATE" };
    SettingsButton settingsButton;

    // 02 filter
    subforge::ui::SegmentedChoice filterMode { param ("filterMode"), { "LP24", "LP12", "BP12", "DIODE" } };
    subforge::ui::FilterCurve filterCurve { processor.apvts };
    Knob envAmount { processor.apvts, "envAmount", "Env", small() };
    Knob filterDecay { processor.apvts, "filterDecay", "Decay", small() };
    Knob drive { processor.apvts, "drive", "Drive", small() };
    Knob keyTrack { processor.apvts, "keyTrack", "Key", small() };
    Knob cutoff { processor.apvts, "cutoff", "Cutoff", { 92, false, 26.0f, 14.0f } };
    Knob resonance { processor.apvts, "resonance", "Resonance", { 92, false, 26.0f, 14.0f } };

    // 01 osc
    subforge::ui::SegmentedChoice wave { param ("wave"), { "SAW", "SQR", "PLS", "SIN", "TRI" } };
    Knob octave { processor.apvts, "octave", "Octave", small() };
    Knob detune { processor.apvts, "detune", "Detune", small() };
    Knob sub { processor.apvts, "sub", "Sub", small() };
    subforge::ui::ChoiceChip subWave { param ("subWave"), { "Sine", "Square" } };

    // 03 drive
    subforge::ui::ChoiceChip distType { param ("distType") };
    Knob distGain { processor.apvts, "distDrive", "Gain", small (true) };
    Knob distTone { processor.apvts, "distTone", "Tone", small (true) };
    Knob distMix { processor.apvts, "distMix", "Mix", small (true) };

    // 04 amp env, 05 pitch
    Knob attack { processor.apvts, "attack", "A", small() };
    Knob decay { processor.apvts, "decay", "D", small() };
    Knob sustain { processor.apvts, "sustain", "S", small() };
    Knob release { processor.apvts, "release", "R", small() };
    Knob pitchEnv { processor.apvts, "pitchEnv", "Env", small() };
    Knob pitchDecay { processor.apvts, "pitchDecay", "Decay", small() };
    Knob click { processor.apvts, "click", "Click", small() };

    // 06 lfo, 07 mod, 09 play
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

    // 08 sequencer
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
    int shownSeqLength = -1, shownSeqRoot = -1;

    std::unique_ptr<juce::FileChooser> folderChooser;
    std::unique_ptr<AboutOverlay> aboutOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
