#include "StepGrid.h"

namespace
{
using P = SubForgeLookAndFeel::Palette;

// rows from the top
constexpr float kLedHeight = 10.0f, kNoteHeight = 18.0f, kChipHeight = 18.0f, kGap = 4.0f;
constexpr int kPixelsPerSemitone = 5;
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

juce::Rectangle<float> StepGrid::cell (int step, Row row) const
{
    const float columnWidth = (float) getWidth() / (float) subforge::kMaxSteps;
    const float x = (float) step * columnWidth + 3.0f;
    const float w = columnWidth - 6.0f;
    const float h = (float) getHeight();
    const float chipsTop = h - 2.0f * kChipHeight - kGap;
    const float noteTop = chipsTop - kGap - kNoteHeight;
    const float barTop = kLedHeight + kGap;

    switch (row)
    {
        case Row::Led:    return { x, 0.0f, w, kLedHeight };
        case Row::Bar:    return { x, barTop, w, noteTop - barTop };
        case Row::Note:   return { x, noteTop, w, kNoteHeight };
        case Row::Accent: return { x, chipsTop, w, kChipHeight };
        case Row::Slide:  return { x, chipsTop + kChipHeight + kGap, w, kChipHeight };
        case Row::None:   break;
    }
    return {};
}

int StepGrid::stepAt (int x) const
{
    return juce::jlimit (0, subforge::kMaxSteps - 1, x * subforge::kMaxSteps / juce::jmax (1, getWidth()));
}

StepGrid::Row StepGrid::rowAt (int y) const
{
    for (auto row : { Row::Bar, Row::Note, Row::Accent, Row::Slide })
    {
        const auto r = cell (0, row);
        if ((float) y >= r.getY() - kGap * 0.5f && (float) y < r.getBottom() + kGap * 0.5f)
            return row == Row::Note ? Row::Bar : row; // the note name belongs to the bar
    }
    return Row::None;
}

void StepGrid::paint (juce::Graphics& g)
{
    const auto accent = lnf.getAccent();
    const int length = (int) processor.apvts.getRawParameterValue ("seqLength")->load();
    const int root = (int) processor.apvts.getRawParameterValue ("seqRoot")->load();
    auto& pattern = processor.getPattern();
    constexpr float noteSpan = (float) (subforge::kMaxStepNote - subforge::kMinStepNote);

    for (int i = 0; i < subforge::kMaxSteps; ++i)
    {
        const auto step = pattern.get (i);
        const bool inPattern = i < length;
        const bool playing = i == playhead;

        // playhead LED
        const auto led = juce::Rectangle<float> (7.0f, 7.0f).withCentre (cell (i, Row::Led).getCentre());
        if (playing)
        {
            g.setColour (accent.withAlpha (0.3f));
            g.fillEllipse (led.expanded (3.0f));
            g.setColour (accent);
        }
        else
        {
            g.setColour (P::track.brighter (inPattern ? 0.4f : 0.0f));
        }
        g.fillEllipse (led);

        // bar box, outlined while playing
        const auto box = cell (i, Row::Bar);
        g.setColour (P::field.darker (0.15f));
        g.fillRoundedRectangle (box, 6.0f);
        g.setColour (playing ? accent : P::panelEdge);
        g.drawRoundedRectangle (box.reduced (0.5f), 6.0f, playing ? 1.5f : 1.0f);

        const auto inner = box.reduced (5.0f);
        if (step.gate)
        {
            const float level = 0.08f + 0.92f * (float) (step.note - subforge::kMinStepNote) / noteSpan;
            const auto bar = inner.withTop (inner.getBottom() - juce::jmax (4.0f, inner.getHeight() * level));
            g.setColour (! inPattern ? P::track : step.accent ? P::secondary : accent);
            g.fillRoundedRectangle (bar, 4.0f);
        }
        else
        {
            g.setColour (P::track);
            g.fillRoundedRectangle (inner.withTop (inner.getBottom() - 3.0f), 1.5f);
        }

        // note name ("..." for a rest)
        g.setColour (inPattern && step.gate ? P::text : P::dimText.withAlpha (inPattern ? 1.0f : 0.6f));
        g.setFont (lnf.titleFont (14.0f));
        g.drawText (step.gate ? juce::MidiMessage::getMidiNoteName (juce::jlimit (0, 127, root + step.note), true, true, 3)
                              : juce::String ("..."),
                    cell (i, Row::Note), juce::Justification::centred);

        // ACC (filled orange) and SLD (outlined in the accent) buttons
        auto drawChip = [&] (Row row, bool on, const char* text, bool filled, juce::Colour colour)
        {
            const auto r = cell (i, row);
            const float alpha = inPattern ? 1.0f : 0.5f;
            if (on && filled)
            {
                g.setColour (colour.withMultipliedAlpha (alpha));
                g.fillRoundedRectangle (r, 4.0f);
            }
            else
            {
                g.setColour (P::field.withMultipliedAlpha (alpha));
                g.fillRoundedRectangle (r, 4.0f);
                g.setColour (on ? colour.withMultipliedAlpha (alpha) : P::panelEdge);
                g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, on ? 1.5f : 1.0f);
            }
            g.setColour (on ? (filled ? P::background : colour) : P::dimText.withMultipliedAlpha (alpha * 0.8f));
            g.setFont (lnf.titleFont (11.0f).withExtraKerningFactor (0.06f));
            g.drawText (text, r, juce::Justification::centred);
        };
        drawChip (Row::Accent, step.accent, "ACC", true, P::secondary);
        drawChip (Row::Slide, step.slide, "SLD", false, accent);
    }
}

void StepGrid::setFlag (int step, Row row, bool value)
{
    auto& pattern = processor.getPattern();
    auto s = pattern.get (step);
    if (row == Row::Accent)
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

void StepGrid::setGate (int step, bool gate)
{
    auto& pattern = processor.getPattern();
    auto s = pattern.get (step);
    s.gate = gate;
    pattern.set (step, s);
    processor.markPatternEdited();
    repaint();
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    dragRow = rowAt (e.y);
    dragStep = stepAt (e.x);
    dragged = false;
    const auto s = processor.getPattern().get (dragStep);

    switch (dragRow)
    {
        case Row::Accent: paintValue = ! s.accent; setFlag (dragStep, dragRow, paintValue); break;
        case Row::Slide:  paintValue = ! s.slide; setFlag (dragStep, dragRow, paintValue); break;
        case Row::Bar:    dragStartNote = s.note; break;
        case Row::Led:
        case Row::Note:
        case Row::None:   break;
    }
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragRow == Row::Bar)
    {
        dragged = dragged || std::abs (e.getDistanceFromDragStartY()) > 3;
        if (dragged)
        {
            if (! processor.getPattern().get (dragStep).gate)
                setGate (dragStep, true); // dragging a rest brings the step back
            setNote (dragStep, dragStartNote - e.getDistanceFromDragStartY() / kPixelsPerSemitone);
        }
    }
    else if (dragRow == Row::Accent || dragRow == Row::Slide)
    {
        const int step = stepAt (e.x);
        const auto s = processor.getPattern().get (step);
        if ((dragRow == Row::Accent ? s.accent : s.slide) != paintValue)
            setFlag (step, dragRow, paintValue);
    }
}

void StepGrid::mouseUp (const juce::MouseEvent& e)
{
    if (dragRow == Row::Bar && ! dragged && e.getNumberOfClicks() < 2)
        setGate (dragStep, ! processor.getPattern().get (dragStep).gate);
    dragRow = Row::None;
}

void StepGrid::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (rowAt (e.y) == Row::Bar)
    {
        const int step = stepAt (e.x);
        setGate (step, true);
        setNote (step, 0);
    }
}

void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (rowAt (e.y) != Row::Bar || std::abs (wheel.deltaY) < 1.0e-6f)
        return;
    const int step = stepAt (e.x);
    setNote (step, processor.getPattern().get (step).note + (wheel.deltaY > 0.0f ? 1 : -1));
}
