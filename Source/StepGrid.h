// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "PluginProcessor.h"
#include "SubForgeLookAndFeel.h"

// The sequencer's 16 steps: a playhead LED, a bar whose height shows the note,
// the note name, and ACC / SLD buttons.
//   bar:      drag up/down or scroll to change the note, click to switch the step
//             on/off (a rest shows "..."), double-click to reset the note to the root
//   ACC/SLD:  click to toggle, drag across steps to paint
// Steps beyond the pattern length are dimmed.
class StepGrid : public juce::Component, public juce::TooltipClient
{
public:
    StepGrid (SubForgeProcessor& processor, SubForgeLookAndFeel& lf);

    void setPlayhead (int step);

    // tooltip for the note bars (and LED and note name) and for the ACC / SLD rows
    void setTooltips (const juce::String& noteTip, const juce::String& flagTip);
    juce::String getTooltip() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    enum class Row { Led, Bar, Note, Accent, Slide, None };

    juce::Rectangle<float> cell (int step, Row row) const;
    int stepAt (int x) const;
    Row rowAt (int y) const;
    void setFlag (int step, Row row, bool value);
    void setNote (int step, int note);
    void setGate (int step, bool gate);

    SubForgeProcessor& processor;
    SubForgeLookAndFeel& lnf;
    int playhead = -1;
    juce::String noteTooltip, flagTooltip;

    Row dragRow = Row::None;
    bool paintValue = false, dragged = false;
    int dragStep = -1, dragStartNote = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGrid)
};
