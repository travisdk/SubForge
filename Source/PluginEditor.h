// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SubForgeLookAndFeel.h"
#include "MainView.h"

// Hosts MainView and scales it between 75 % and 200 % of its design size,
// either from the gear menu or by dragging the resize corner. The chosen size
// is remembered per host and shrunk only as far as needed to fit the screen.
// A size forced by the host is letterboxed rather than remembered.
class SubForgeEditor : public juce::AudioProcessorEditor
{
public:
    explicit SubForgeEditor (SubForgeProcessor&);
    ~SubForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setScaleFactor (float newScale) override;
    void parentHierarchyChanged() override;

private:
    // Keeps the aspect ratio while the resize corner is dragged, but takes a size set by the
    // host as it is: Ardour cannot shrink its plug-in window below the size it opened at, and
    // forcing the aspect ratio onto that size pushed part of the interface out of view.
    struct Constrainer : juce::ComponentBoundsConstrainer
    {
        void checkBounds (juce::Rectangle<int>& bounds, const juce::Rectangle<int>& previous,
                          const juce::Rectangle<int>& limits, bool top, bool left, bool bottom, bool right) override;
    };

    float currentScale() const;
    void setUiScale (float scale);
    void applyLayout (float scale); // size, limits and aspect ratio for the view's current design height

    SubForgeProcessor& processor;
    const juce::String scaleKey; // settings key for this host's size
    float preferredScale = 1.0f; // the user's choice, before fitting it to the screen
    bool applyingLayout = false;
    Constrainer constrainer;
    SubForgeLookAndFeel lookAndFeel;
    std::unique_ptr<MainView> view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubForgeEditor)
};
