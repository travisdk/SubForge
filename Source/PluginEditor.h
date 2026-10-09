#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class SubForgeEditor : public juce::AudioProcessorEditor,
                       private juce::ChangeListener
{
public:
    explicit SubForgeEditor (SubForgeProcessor&);
    ~SubForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Control
    {
        juce::Label label;
        juce::Slider slider;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> boxAttachment;
        bool isChoice = false;
    };

    struct Section
    {
        juce::String title;
        std::vector<Control*> controls;
        juce::Rectangle<int> bounds;
    };

    Control* addKnob (const char* paramId, const juce::String& text);
    Control* addChoice (const char* paramId, const juce::String& text);
    void layoutSection (Section& section, juce::Rectangle<int> area);
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void refreshPresetBox();
    void stepPreset (int delta);

    SubForgeProcessor& processor;
    juce::LookAndFeel_V4 lookAndFeel;

    std::vector<std::unique_ptr<Control>> controls;
    std::vector<Section> sections;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TextButton randomButton { "Randomize" }, mutateButton { "Mutate" };
    juce::Rectangle<int> headerArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubForgeEditor)
};
