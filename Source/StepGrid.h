#pragma once

#include "PluginProcessor.h"
#include "SubForgeLookAndFeel.h"

// The sequencer's 16 step columns: playhead LED, gate, note, accent and slide.
// Click toggles (drag to paint across steps), drag or scroll the note to
// change it, double-click the note to reset it to the root.
class StepGrid : public juce::Component
{
public:
    StepGrid (SubForgeProcessor& processor, SubForgeLookAndFeel& lf);

    void setPlayhead (int step);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    enum class Row { Led, Gate, Note, Accent, Slide, None };

    juce::Rectangle<int> cellBounds (int step, Row row) const;
    int stepAt (int x) const;
    Row rowAt (int y) const;
    void paintFlag (int step, Row row, bool value);
    void setNote (int step, int note);

    SubForgeProcessor& processor;
    SubForgeLookAndFeel& lnf;
    int playhead = -1;

    Row dragRow = Row::None;
    bool paintValue = false;
    int dragStep = -1, dragStartNote = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGrid)
};
