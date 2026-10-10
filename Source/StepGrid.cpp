#include "StepGrid.h"

namespace
{
using P = SubForgeLookAndFeel::Palette;

// row layout from the top: LED, gate, note, accent, slide
constexpr int kLedHeight = 8, kGateHeight = 20, kNoteHeight = 22, kFlagHeight = 16, kGap = 4;
constexpr int kPixelsPerSemitone = 6;
} // namespace

StepGrid::StepGrid (SubForgeProcessor& p, SubForgeLookAndFeel& lf) : processor (p), lnf (lf) {}

void StepGrid::setPlayhead (int step)
{
    if (step != playhead)
    {
        playhead = step;
        repaint();
    }
}

juce::Rectangle<int> StepGrid::cellBounds (int step, Row row) const
{
    const float columnWidth = (float) getWidth() / (float) subforge::kMaxSteps;
    const int x = juce::roundToInt ((float) step * columnWidth);
    const int w = juce::roundToInt ((float) (step + 1) * columnWidth) - x;

    int y = 0, h = 0;
    switch (row)
    {
        case Row::Led:    y = 0; h = kLedHeight; break;
        case Row::Gate:   y = kLedHeight + kGap; h = kGateHeight; break;
        case Row::Note:   y = kLedHeight + kGateHeight + 2 * kGap; h = kNoteHeight; break;
        case Row::Accent: y = kLedHeight + kGateHeight + kNoteHeight + 3 * kGap; h = kFlagHeight; break;
        case Row::Slide:  y = kLedHeight + kGateHeight + kNoteHeight + kFlagHeight + 4 * kGap; h = kFlagHeight; break;
        case Row::None:   break;
    }
    return { x + 2, y, w - 4, h };
}

int StepGrid::stepAt (int x) const
{
    return juce::jlimit (0, subforge::kMaxSteps - 1, x * subforge::kMaxSteps / juce::jmax (1, getWidth()));
}

StepGrid::Row StepGrid::rowAt (int y) const
{
    for (auto row : { Row::Gate, Row::Note, Row::Accent, Row::Slide })
    {
        const auto cell = cellBounds (0, row);
        if (y >= cell.getY() - kGap / 2 && y < cell.getBottom() + kGap / 2)
            return row;
    }
    return Row::None;
}

void StepGrid::paint (juce::Graphics& g)
{
    const auto accent = lnf.getAccent();
    const int length = (int) processor.apvts.getRawParameterValue ("seqLength")->load();
    const int root = (int) processor.apvts.getRawParameterValue ("seqRoot")->load();
    auto& pattern = processor.getPattern();

    for (int i = 0; i < subforge::kMaxSteps; ++i)
    {
        const auto step = pattern.get (i);
        const bool active = i < length;
        const float alpha = active ? 1.0f : 0.3f;

        // playhead LED, with a brighter tick on every beat
        const auto led = cellBounds (i, Row::Led).toFloat();
        const auto dot = juce::Rectangle<float> (6.0f, 6.0f).withCentre (led.getCentre());
        g.setColour (i == playhead ? accent : (i % 4 == 0 ? P::dimText : P::track).withMultipliedAlpha (alpha));
        g.fillEllipse (dot);

        const auto gate = cellBounds (i, Row::Gate).toFloat();
        g.setColour ((step.gate ? accent.withAlpha (0.85f) : P::field).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (gate, 3.0f);
        if (i == playhead && step.gate)
        {
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillRoundedRectangle (gate, 3.0f);
        }

        const auto noteCell = cellBounds (i, Row::Note).toFloat();
        g.setColour (P::field.withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (noteCell, 3.0f);
        g.setColour ((step.gate ? P::text : P::dimText).withMultipliedAlpha (alpha));
        g.setFont (lnf.regularFont (11.0f));
        g.drawText (juce::MidiMessage::getMidiNoteName (juce::jlimit (0, 127, root + step.note), true, true, 3),
                    noteCell.toNearestInt(), juce::Justification::centred);

        auto drawFlag = [&] (Row row, bool on, const char* letter)
        {
            const auto cell = cellBounds (i, row).toFloat();
            g.setColour ((on ? accent : P::field).withMultipliedAlpha (alpha));
            g.fillRoundedRectangle (cell, 3.0f);
            g.setColour ((on ? P::background : P::dimText).withMultipliedAlpha (alpha));
            g.setFont (lnf.boldFont (10.0f));
            g.drawText (letter, cell.toNearestInt(), juce::Justification::centred);
        };
        drawFlag (Row::Accent, step.accent, "A");
        drawFlag (Row::Slide, step.slide, "S");
    }
}

void StepGrid::paintFlag (int step, Row row, bool value)
{
    auto& pattern = processor.getPattern();
    auto s = pattern.get (step);
    if (row == Row::Gate)
        s.gate = value;
    else if (row == Row::Accent)
        s.accent = value;
    else if (row == Row::Slide)
        s.slide = value;
    pattern.set (step, s);
    processor.markPatternEdited();
    repaint();
}

void StepGrid::setNote (int step, int note)
{
    auto& pattern = processor.getPattern();
    auto s = pattern.get (step);
    s.note = juce::jlimit (subforge::kMinStepNote, subforge::kMaxStepNote, note);
    pattern.set (step, s);
    processor.markPatternEdited();
    repaint();
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    dragRow = rowAt (e.y);
    dragStep = stepAt (e.x);
    const auto s = processor.getPattern().get (dragStep);

    switch (dragRow)
    {
        case Row::Gate:   paintValue = ! s.gate; break;
        case Row::Accent: paintValue = ! s.accent; break;
        case Row::Slide:  paintValue = ! s.slide; break;
        case Row::Note:   dragStartNote = s.note; return;
        case Row::Led:
        case Row::None:   return;
    }
    paintFlag (dragStep, dragRow, paintValue);
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragRow == Row::Note)
        setNote (dragStep, dragStartNote + (-e.getDistanceFromDragStartY()) / kPixelsPerSemitone);
    else if (dragRow == Row::Gate || dragRow == Row::Accent || dragRow == Row::Slide)
        paintFlag (stepAt (e.x), dragRow, paintValue);
}

void StepGrid::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (rowAt (e.y) == Row::Note)
        setNote (stepAt (e.x), 0);
}

void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (rowAt (e.y) != Row::Note || std::abs (wheel.deltaY) < 1.0e-6f)
        return;
    const int step = stepAt (e.x);
    setNote (step, processor.getPattern().get (step).note + (wheel.deltaY > 0.0f ? 1 : -1));
}
