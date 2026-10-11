// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SubForgeLookAndFeel.h"

// Building blocks of the Syre editor. Each control is bound to one parameter
// and finds the SubForge look-and-feel through its parent hierarchy.
namespace subforge::ui
{

// The SubForge look of a component, or nullptr before it is attached to the view.
SubForgeLookAndFeel* lookOf (const juce::Component&);

// A rotary knob with its name above and value below.
class Knob : public juce::Component
{
public:
    struct Options
    {
        int knobSize = 36;
        bool secondary = false;    // orange accent (drive section)
        float valueSize = 12.5f;
        float labelSize = 11.0f;
    };

    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& name, Options);

    // Replaces the parameter's value text, e.g. short names for choices.
    void setValueText (std::function<juce::String (double)> toText);

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::Slider slider;

private:
    int valueHeight() const;

public:

private:
    juce::String name;
    Options options;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Mutually exclusive buttons for a choice parameter (filter mode, wave).
class SegmentedChoice : public juce::Component
{
public:
    SegmentedChoice (juce::RangedAudioParameter&, const juce::StringArray& labels);
    void resized() override;

private:
    void showIndex (int index);

    juce::OwnedArray<juce::TextButton> buttons;
    juce::ParameterAttachment attachment;
};

// A chip showing a choice parameter's current value. Two-value parameters toggle
// on click; others open a menu.
class ChoiceChip : public juce::TextButton
{
public:
    ChoiceChip (juce::RangedAudioParameter&, juce::StringArray displayNames = {});
    using juce::TextButton::clicked;
    void clicked() override;

private:
    juce::RangedAudioParameter& param;
    juce::StringArray names;
    juce::ParameterAttachment attachment;
};

// "LABEL value" chip. Drag vertically or use the wheel to change the value;
// choice parameters also step forward on click.
class ValueChip : public juce::Component
{
public:
    ValueChip (juce::RangedAudioParameter&, const juce::String& label);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    void stepBy (int steps);

    juce::RangedAudioParameter& param;
    juce::String label;
    juce::ParameterAttachment attachment;
    float dragStartNormalised = 0.0f;
    bool dragged = false;
};

// Live magnitude response of the filter, recomputed on the message thread
// (at most 30 fps, only when cutoff, resonance or mode changed).
class FilterCurve : public juce::Component, private juce::Timer
{
public:
    explicit FilterCurve (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void resized() override { rebuild(); }

private:
    void timerCallback() override;
    void rebuild();

    juce::AudioProcessorValueTreeState& state;
    std::atomic<float>* cutoff;
    std::atomic<float>* resonance;
    std::atomic<float>* mode;
    float shownCutoff = -1.0f, shownResonance = -1.0f, shownMode = -1.0f;
    juce::Path curve, area;
    juce::Point<float> peak;
};

} // namespace subforge::ui
