#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SubForgeLookAndFeel.h"
#include "MainView.h"

// Hosts MainView and scales it between 75 % and 200 % of its design size,
// either from the gear menu or by dragging the resize corner.
class SubForgeEditor : public juce::AudioProcessorEditor
{
public:
    explicit SubForgeEditor (SubForgeProcessor&);
    ~SubForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    float currentScale() const { return (float) getWidth() / (float) MainView::kWidth; }
    void setUiScale (float scale);
    void applyLayout (float scale); // size, limits and aspect ratio for the view's current design height

    SubForgeProcessor& processor;
    SubForgeLookAndFeel lookAndFeel;
    std::unique_ptr<MainView> view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubForgeEditor)
};
