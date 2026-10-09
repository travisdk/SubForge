#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// SubForge's own look: hardware-style knobs with a thin value ring, flat
// fields, LED toggles and the embedded Inter typeface.
class SubForgeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    struct Palette
    {
        static inline const juce::Colour background { 0xff17191d };
        static inline const juce::Colour panel { 0xff22252b };
        static inline const juce::Colour panelEdge { 0xff2e323a };
        static inline const juce::Colour field { 0xff2b2f37 };
        static inline const juce::Colour track { 0xff343842 };
        static inline const juce::Colour knobTop { 0xff464b57 };
        static inline const juce::Colour knobBottom { 0xff24272e };
        static inline const juce::Colour text { 0xffe8e8ea };
        static inline const juce::Colour dimText { 0xff8d939e };
    };

    static constexpr int kKnobSize = 50;

    struct AccentChoice
    {
        const char* name;
        juce::Colour colour;
    };
    static const std::vector<AccentChoice>& accentChoices(); // first entry is the default

    SubForgeLookAndFeel();

    juce::Colour getAccent() const { return accent; }
    void setAccent (juce::Colour newAccent);

    juce::Font regularFont (float height) const { return juce::Font (medium).withHeight (height); }
    juce::Font boldFont (float height) const { return juce::Font (bold).withHeight (height); }

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return regularFont (13.0f); }

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon,
                            const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override { return regularFont (14.0f); }
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>& area,
                                     const juce::String& sectionName) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return boldFont (13.0f); }

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool isHighlighted, bool isDown) override;

    juce::Font getLabelFont (juce::Label&) override { return regularFont (12.5f); }

private:
    juce::Typeface::Ptr medium, bold;
    juce::Colour accent;
};
