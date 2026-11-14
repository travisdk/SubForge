// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "SubForgeLookAndFeel.h"

// "About SubForge": a small rounded panel inside the editor (no dialog), with the version, the author, two links and a
// Licences button that swaps the panel for the full third-party notices. Close it with the cross, Esc or a click outside.
class AboutOverlay : public juce::Component
{
public:
    AboutOverlay (SubForgeLookAndFeel& lf, std::function<void()> onClose);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusText();

private:
    juce::Rectangle<int> panelBounds() const;
    juce::Rectangle<int> crossArea() const;
    void setShowingLicences (bool show);

    SubForgeLookAndFeel& lnf;
    std::function<void()> close;
    juce::HyperlinkButton homepage, project;
    juce::TextButton licencesButton { "Licences" };
    juce::TextEditor notices;
    bool showingLicences = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutOverlay)
};
