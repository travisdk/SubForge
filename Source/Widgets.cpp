// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#include "Widgets.h"
#include "FilterResponse.h"

namespace subforge::ui
{
using P = SubForgeLookAndFeel::Palette;

SubForgeLookAndFeel* lookOf (const juce::Component& c)
{
    return dynamic_cast<SubForgeLookAndFeel*> (&c.getLookAndFeel());
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& displayName,
            Options opts)
    : name (displayName.toUpperCase()), options (opts)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0); // the knob paints its value itself
    slider.onValueChange = [this] { repaint(); };
    slider.getProperties().set (SubForgeLookAndFeel::knobSizeProperty, options.knobSize);
    slider.getProperties().set (SubForgeLookAndFeel::secondaryProperty, options.secondary);
    slider.getProperties().set (SubForgeLookAndFeel::valueSizeProperty, options.valueSize);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);

    auto* param = state.getParameter (paramId);
    slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    // Fine adjustment: dragging while holding Shift (or Ctrl/Alt/Cmd) follows the speed of the
    // mouse, so slow movements change the value in tiny steps. The mouse wheel is JUCE's own.
    slider.setVelocityModeParameters (1.0, 1, 0.0, true,
                                      (juce::ModifierKeys::Flags) (juce::ModifierKeys::shiftModifier
                                                                   | juce::ModifierKeys::ctrlAltCommandModifiers));
    addAndMakeVisible (slider);
}

void Knob::setValueText (std::function<juce::String (double)> toText)
{
    slider.textFromValueFunction = std::move (toText);
    repaint();
}

int Knob::valueHeight() const
{
    return (int) options.valueSize + 8;
}

void Knob::paint (juce::Graphics& g)
{
    auto* look = lookOf (*this);
    if (look == nullptr)
        return;
    g.setColour (P::dimText);
    g.setFont (look->labelFont (options.labelSize).withExtraKerningFactor (0.1f));
    g.drawText (name, getLocalBounds().removeFromTop ((int) options.labelSize + 6), juce::Justification::centred);

    // value below the knob; long values are squeezed rather than cut off
    g.setColour (slider.isEnabled() ? P::text : P::dimText);
    g.setFont (look->valueFont (options.valueSize));
    g.drawFittedText (slider.getTextFromValue (slider.getValue()), getLocalBounds().removeFromBottom (valueHeight()),
                      juce::Justification::centred, 1, 0.5f);
}

void Knob::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop ((int) options.labelSize + 6);
    area.removeFromBottom (valueHeight());
    slider.setBounds (area);
}

//==============================================================================
SegmentedChoice::SegmentedChoice (juce::RangedAudioParameter& param, const juce::StringArray& labels)
    : attachment (param, [this] (float value) { showIndex (juce::roundToInt (value)); })
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new juce::TextButton (labels[i]));
        b->setClickingTogglesState (false);
        b->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    attachment.sendInitialUpdate();
}

void SegmentedChoice::showIndex (int index)
{
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (i == index, juce::dontSendNotification);
}

void SegmentedChoice::resized()
{
    constexpr int gap = 5;
    const int n = buttons.size();
    const int w = (getWidth() - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i)
        buttons[i]->setBounds (i * (w + gap), 0, i == n - 1 ? getWidth() - i * (w + gap) : w, getHeight());
}

//==============================================================================
ChoiceChip::ChoiceChip (juce::RangedAudioParameter& p, juce::StringArray displayNames)
    : param (p),
      names (displayNames.isEmpty() ? dynamic_cast<juce::AudioParameterChoice&> (p).choices : displayNames),
      attachment (p, [this] (float value) { setButtonText (names[juce::roundToInt (value)].toUpperCase()); })
{
    attachment.sendInitialUpdate();
}

void ChoiceChip::clicked()
{
    const int current = juce::roundToInt (param.convertFrom0to1 (param.getValue()));
    if (names.size() == 2)
    {
        attachment.setValueAsCompleteGesture ((float) (1 - current));
        return;
    }

    juce::PopupMenu menu;
    for (int i = 0; i < names.size(); ++i)
        menu.addItem (names[i], true, i == current, [this, i] { attachment.setValueAsCompleteGesture ((float) i); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withDeletionCheck (*this));
}

//==============================================================================
ValueChip::ValueChip (juce::RangedAudioParameter& p, const juce::String& chipLabel)
    : param (p), label (chipLabel.toUpperCase()), attachment (p, [this] (float) { repaint(); })
{
    attachment.sendInitialUpdate();
    setRepaintsOnMouseActivity (true);
}

void ValueChip::paint (juce::Graphics& g)
{
    auto* lookPtr = lookOf (*this);
    if (lookPtr == nullptr)
        return;
    auto& look = *lookPtr;
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (P::field);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (isMouseOverOrDragging() ? P::panelEdge.brighter (0.5f) : P::panelEdge.brighter (0.15f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    const auto labelFont = look.labelFont (11.0f).withExtraKerningFactor (0.1f);
    const auto valueFont = look.valueFont (13.0f);
    const auto value = param.getCurrentValueAsText();
    const float labelW = juce::GlyphArrangement::getStringWidth (labelFont, label);
    const float valueW = juce::GlyphArrangement::getStringWidth (valueFont, value);
    const float x = ((float) getWidth() - labelW - 6.0f - valueW) * 0.5f;

    g.setColour (P::dimText);
    g.setFont (labelFont);
    g.drawText (label, juce::Rectangle<float> (x, 0.0f, labelW + 1.0f, (float) getHeight()), juce::Justification::centredLeft);
    g.setColour (P::text);
    g.setFont (valueFont);
    g.drawText (value, juce::Rectangle<float> (x + labelW + 6.0f, 0.0f, valueW + 2.0f, (float) getHeight()),
                juce::Justification::centredLeft);
}

void ValueChip::mouseDown (const juce::MouseEvent&)
{
    dragStartNormalised = param.getValue();
    dragged = false;
    attachment.beginGesture();
}

void ValueChip::mouseDrag (const juce::MouseEvent& e)
{
    dragged = dragged || std::abs (e.getDistanceFromDragStartY()) > 2;
    if (! dragged)
        return;
    const float normalised = juce::jlimit (0.0f, 1.0f, dragStartNormalised - (float) e.getDistanceFromDragStartY() / 150.0f);
    attachment.setValueAsPartOfGesture (param.convertFrom0to1 (normalised));
}

void ValueChip::mouseUp (const juce::MouseEvent&)
{
    attachment.endGesture();
    if (! dragged && param.getNumSteps() < 64) // a click steps choices and small ranges forward, wrapping
    {
        const auto range = param.getNormalisableRange();
        const float next = param.convertFrom0to1 (param.getValue()) + (range.interval > 0.0f ? range.interval : 1.0f);
        attachment.setValueAsCompleteGesture (next > range.end ? range.start : next);
    }
}

void ValueChip::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (std::abs (wheel.deltaY) > 1.0e-6f)
        stepBy (wheel.deltaY > 0.0f ? 1 : -1);
}

void ValueChip::stepBy (int steps)
{
    const auto range = param.getNormalisableRange();
    const float step = range.interval > 0.0f ? range.interval : (range.end - range.start) / 100.0f;
    attachment.setValueAsCompleteGesture (juce::jlimit (range.start, range.end, param.convertFrom0to1 (param.getValue()) + step * (float) steps));
}

//==============================================================================
FilterCurve::FilterCurve (juce::AudioProcessorValueTreeState& s)
    : state (s),
      cutoff (s.getRawParameterValue ("cutoff")),
      resonance (s.getRawParameterValue ("resonance")),
      mode (s.getRawParameterValue ("filterMode"))
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void FilterCurve::timerCallback()
{
    if (! juce::exactlyEqual (cutoff->load(), shownCutoff) || ! juce::exactlyEqual (resonance->load(), shownResonance)
        || ! juce::exactlyEqual (mode->load(), shownMode))
        rebuild();
}

void FilterCurve::rebuild()
{
    shownCutoff = cutoff->load();
    shownResonance = resonance->load();
    shownMode = mode->load();

    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    constexpr float minHz = 20.0f, maxHz = 20000.0f, topDb = 18.0f, bottomDb = -42.0f;
    auto xFor = [&] (float hz) { return bounds.getX() + bounds.getWidth() * std::log (hz / minHz) / std::log (maxHz / minHz); };
    auto yFor = [&] (float db) { return bounds.getY() + bounds.getHeight() * (topDb - juce::jlimit (bottomDb, topDb, db)) / (topDb - bottomDb); };

    curve.clear();
    float bestDb = -1000.0f;
    constexpr int points = 180;
    for (int i = 0; i <= points; ++i)
    {
        const float hz = minHz * std::pow (maxHz / minHz, (float) i / points);
        const float db = juce::Decibels::gainToDecibels (subforge::filterMagnitude ((int) shownMode, shownCutoff, shownResonance, hz), -100.0f);
        const juce::Point<float> pt { xFor (hz), yFor (db) };
        i == 0 ? curve.startNewSubPath (pt) : curve.lineTo (pt);
        if (db > bestDb)
        {
            bestDb = db;
            peak = pt;
        }
    }

    area = curve;
    area.lineTo (bounds.getBottomRight());
    area.lineTo (bounds.getBottomLeft());
    area.closeSubPath();
    repaint();
}

void FilterCurve::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    auto* look = lookOf (*this);
    const auto accent = look != nullptr ? look->getAccent() : juce::Colours::white;

    g.setColour (P::background.interpolatedWith (P::panel, 0.5f));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (P::panelEdge);
    for (int i = 1; i < 4; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * (float) i / 4.0f;
        g.drawVerticalLine ((int) x, bounds.getY() + 2.0f, bounds.getBottom() - 2.0f);
    }
    g.drawHorizontalLine ((int) (bounds.getY() + bounds.getHeight() * 0.5f), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

    g.reduceClipRegion (getLocalBounds().reduced (1));
    g.setColour (accent.withAlpha (0.12f));
    g.fillPath (area);
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (peak));

    g.setColour (P::panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
}

} // namespace subforge::ui
