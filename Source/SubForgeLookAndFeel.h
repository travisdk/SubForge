// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// SubForge's "Syre" look: dark panels, lime accent with an orange secondary,
// vector knobs (arc + pointer), Chakra Petch for labels and JetBrains Mono for values.
class SubForgeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    struct Palette
    {
        static inline const juce::Colour background { 0xff0b0d0c };
        static inline const juce::Colour panel { 0xff141716 };
        static inline const juce::Colour panelEdge { 0xff232826 };
        static inline const juce::Colour field { 0xff1c201e };
        static inline const juce::Colour track { 0xff2a302d };
        static inline const juce::Colour knobBody { 0xff1c201e };
        static inline const juce::Colour text { 0xffe8ede6 };
        static inline const juce::Colour dimText { 0xff8a938d };
        static inline const juce::Colour secondary { 0xffff9f43 }; // accent steps, drive section
    };

    static constexpr int kKnobSize = 40;

    // Per-component properties the look understands
    static inline const juce::Identifier knobSizeProperty { "knobSize" };       // int, max knob diameter
    static inline const juce::Identifier secondaryProperty { "secondaryAccent" }; // bool, use the orange accent
    static inline const juce::Identifier outlineProperty { "accentOutline" };     // bool, button outlined in the accent
    static inline const juce::Identifier valueSizeProperty { "valueFontSize" };   // float, knob value text height
    static inline const juce::Identifier switchProperty { "slideSwitch" };       // bool, draw a toggle as an ON/OFF slide switch

    struct AccentChoice
    {
        const char* name;
        juce::Colour colour;
    };
    static const std::vector<AccentChoice>& accentChoices(); // first entry is the default

    SubForgeLookAndFeel();

    juce::Colour getAccent() const { return accent; }
    void setAccent (juce::Colour newAccent);
    juce::Colour accentFor (const juce::Component& c) const;

    juce::Font labelFont (float height) const { return juce::Font (juce::FontOptions (labelFace).withHeight (height)); }
    juce::Font titleFont (float height) const { return juce::Font (juce::FontOptions (titleFace).withHeight (height)); }
    juce::Font valueFont (float height) const { return juce::Font (juce::FontOptions (valueFace).withHeight (height)); }

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return labelFont (14.0f); }

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon,
                            const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override { return labelFont (14.0f); }
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>& area,
                                     const juce::String& sectionName) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return titleFont (13.0f); }
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isHighlighted, bool isDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool isHighlighted, bool isDown) override;
    void drawSlideSwitch (juce::Graphics&, juce::ToggleButton&, bool isHighlighted);

    // labels keep the font they were given (value boxes use JetBrains Mono at their own size)
    juce::Font getLabelFont (juce::Label& label) override { return label.getFont(); }

    juce::Font getAlertWindowTitleFont() override { return titleFont (16.0f); }
    juce::Font getAlertWindowMessageFont() override { return labelFont (14.0f); }
    juce::Font getAlertWindowFont() override { return labelFont (14.0f); }

private:
    juce::Typeface::Ptr labelFace, titleFace, valueFace;
    juce::Colour accent;
};
