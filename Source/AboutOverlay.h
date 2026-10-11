// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "SubForgeLookAndFeel.h"

// "About SubForge" panel shown on top of the editor: version, copyright and
// the full third-party notices. Click outside, press Escape or Close to dismiss.
class AboutOverlay : public juce::Component
{
public:
    AboutOverlay (SubForgeLookAndFeel& lf, std::function<void()> onClose);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusText() { notices.grabKeyboardFocus(); }

private:
    juce::Rectangle<int> panelBounds() const;

    SubForgeLookAndFeel& lnf;
    std::function<void()> close;
    juce::TextEditor notices;
    juce::TextButton closeButton { "Close" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutOverlay)
};
