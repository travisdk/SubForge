#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SubForgeLookAndFeel.h"
#include "StepGrid.h"
#include "AboutOverlay.h"

class SubForgeEditor : public juce::AudioProcessorEditor,
                       private juce::ChangeListener,
                       private juce::Timer
{
public:
    explicit SubForgeEditor (SubForgeProcessor&);
    ~SubForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Small gear icon that opens the settings menu.
    struct SettingsButton : juce::Button
    {
        SettingsButton() : juce::Button ("Settings") {}
        void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;
    };

    enum class Kind { Knob, Choice, Toggle };

    struct Control
    {
        Kind kind = Kind::Knob;
        juce::Label label;
        juce::Slider slider;
        juce::ComboBox box;
        juce::ToggleButton toggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> boxAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;

        // relative width; drop-downs need more room for their text than knobs
        int weight() const { return kind == Kind::Choice ? 14 : 10; }
    };

    struct Section
    {
        juce::String title;
        std::vector<Control*> controls;
        juce::Rectangle<int> bounds {};

        int weight() const
        {
            int total = 0;
            for (auto* control : controls)
                total += control->weight();
            return total;
        }
    };

    Control* addControl (Kind kind, const char* paramId, const juce::String& text);
    Control* knob (const char* paramId, const juce::String& text) { return addControl (Kind::Knob, paramId, text); }
    Control* choice (const char* paramId, const juce::String& text) { return addControl (Kind::Choice, paramId, text); }
    Control* toggle (const char* paramId, const juce::String& text) { return addControl (Kind::Toggle, paramId, text); }

    void layoutRow (std::initializer_list<Section*> row, juce::Rectangle<int> area);
    void layoutSection (Section& section, juce::Rectangle<int> area);
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refreshPresetBox();
    void stepPreset (int delta);
    void showSettingsMenu();
    void applyAccent (juce::Colour accent);
    void showNameDialog (const juce::String& title, const juce::File& folder, const juce::String& suggestion,
                         std::function<void (const juce::String&)> onName);
    void showSaveDialog();
    void savePreset (const juce::String& name);
    void refreshPatternBox();
    void stepPattern (int delta);
    void showPatternMenu();
    void showAbout();
    void savePattern (const juce::String& name);
    void confirm (const juce::String& title, const juce::String& message, const juce::String& okText,
                  std::function<void()> onOk);
    void choosePresetFolder();

    SubForgeProcessor& processor;
    SubForgeLookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Control>> controls;
    std::vector<Section> sections;
    Control* lfoRateControl = nullptr;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton randomButton { "Randomize" }, mutateButton { "Mutate" };
    SettingsButton settingsButton;
    std::unique_ptr<juce::FileChooser> folderChooser;

    StepGrid stepGrid { processor, lookAndFeel };
    juce::ComboBox patternBox;
    juce::TextButton patternPrevButton { "<" }, patternNextButton { ">" }, patternSaveButton { "Save" };
    juce::TextButton seqRandomButton { "Random" }, patternMenuButton { "..." };
    int shownSeqLength = -1, shownSeqRoot = -1;

    std::unique_ptr<AboutOverlay> aboutOverlay;
    juce::Rectangle<int> headerArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubForgeEditor)
};
