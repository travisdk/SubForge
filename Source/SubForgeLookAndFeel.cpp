#include "SubForgeLookAndFeel.h"
#include "BinaryData.h"

using P = SubForgeLookAndFeel::Palette;

SubForgeLookAndFeel::SubForgeLookAndFeel()
{
    medium = juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_otf, BinaryData::InterMedium_otfSize);
    bold = juce::Typeface::createSystemTypefaceFor (BinaryData::InterBold_otf, BinaryData::InterBold_otfSize);

    setColour (juce::ResizableWindow::backgroundColourId, P::background);
    setColour (juce::Label::textColourId, P::dimText);
    setColour (juce::Slider::textBoxTextColourId, P::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, P::accent.withAlpha (0.4f));
    setColour (juce::TextEditor::backgroundColourId, P::field);
    setColour (juce::TextEditor::textColourId, P::text);
    setColour (juce::TextEditor::highlightColourId, P::accent.withAlpha (0.4f));
    setColour (juce::TextEditor::focusedOutlineColourId, P::accent);
    setColour (juce::CaretComponent::caretColourId, P::accent);
    setColour (juce::ComboBox::textColourId, P::text);
    setColour (juce::PopupMenu::textColourId, P::text);
    setColour (juce::PopupMenu::highlightedTextColourId, P::text);
    setColour (juce::TextButton::buttonColourId, P::field);
    setColour (juce::TextButton::textColourOffId, P::text);
    setColour (juce::TextButton::textColourOnId, P::text);
}

void SubForgeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                            float startAngle, float endAngle, juce::Slider& slider)
{
    const float size = (float) juce::jmin (width, height, kKnobSize);
    const auto centre = juce::Rectangle<int> (x, y, width, height).toFloat().getCentre();
    const float ringRadius = size * 0.5f - 2.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const bool hot = slider.isEnabled() && slider.isMouseOverOrDragging();
    const juce::PathStrokeType ringStroke (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (P::track);
    g.strokePath (track, ringStroke);

    // bipolar ranges (pitch env, volume around 0 dB...) fill from their zero point
    float fromAngle = startAngle;
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
        fromAngle = startAngle + (float) slider.valueToProportionOfLength (0.0) * (endAngle - startAngle);

    if (std::abs (angle - fromAngle) > 0.01f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                             juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
        g.setColour (hot ? P::accent.brighter (0.25f) : P::accent);
        g.strokePath (value, ringStroke);
    }

    const float bodyRadius = ringRadius - 6.0f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (body.translated (0.0f, 1.5f).expanded (0.5f));
    g.setGradientFill (juce::ColourGradient (P::knobTop, centre.x, body.getY(),
                                             P::knobBottom, centre.x, body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colours::white.withAlpha (hot ? 0.14f : 0.08f));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    juce::Path pointer;
    pointer.startNewSubPath (centre.getPointOnCircumference (bodyRadius * 0.25f, angle));
    pointer.lineTo (centre.getPointOnCircumference (bodyRadius * 0.8f, angle));
    g.setColour (P::text);
    g.strokePath (pointer, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Label* SubForgeLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (regularFont (12.0f));
    label->setColour (juce::Label::textColourId, P::text);
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

void SubForgeLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                                        juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (width, height).toFloat().reduced (0.5f);
    const bool hot = box.isEnabled() && (box.isMouseOver (true) || box.isPopupActive());

    g.setColour (P::field);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (hot ? P::accent.withAlpha (0.7f) : P::panelEdge.brighter (0.15f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    const float cx = (float) width - 12.0f, cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
    chevron.lineTo (cx, cy + 2.0f);
    chevron.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (P::accent);
    g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SubForgeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void SubForgeLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (P::panel);
    g.setColour (P::panelEdge.brighter (0.2f));
    g.drawRect (0, 0, width, height, 1);
}

void SubForgeLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator,
                                             bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                                             const juce::String& text, const juce::String& shortcutKeyText,
                                             const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator || hasSubMenu || icon != nullptr || shortcutKeyText.isNotEmpty())
    {
        LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive, isHighlighted, isTicked, hasSubMenu,
                                           text, shortcutKeyText, icon, textColour);
        return;
    }

    auto r = area.reduced (3, 1);
    if (isHighlighted && isActive)
    {
        g.setColour (P::accent.withAlpha (0.22f));
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
    }

    if (isTicked)
    {
        g.setColour (P::accent);
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ (float) r.getX() + 10.0f, (float) r.getCentreY() }));
    }

    g.setColour (isActive ? (isTicked ? P::accent : P::text) : P::dimText);
    g.setFont (getPopupMenuFont());
    g.drawFittedText (text, r.withTrimmedLeft (20).withTrimmedRight (6), juce::Justification::centredLeft, 1);
}

void SubForgeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour, bool isHighlighted, bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto fill = backgroundColour;
    if (isDown)
        fill = fill.darker (0.2f);
    else if (isHighlighted)
        fill = fill.brighter (0.12f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void SubForgeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool isHighlighted, bool)
{
    const auto centre = button.getLocalBounds().toFloat().getCentre();
    const auto led = juce::Rectangle<float> (14.0f, 14.0f).withCentre (centre);

    if (button.getToggleState())
    {
        g.setColour (P::accent.withAlpha (0.18f));
        g.fillEllipse (led.expanded (5.0f));
        g.setColour (P::accent);
        g.fillEllipse (led);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.fillEllipse (led.reduced (4.0f).translated (-1.5f, -1.5f));
    }
    else
    {
        g.setColour (P::field);
        g.fillEllipse (led);
        g.setColour (isHighlighted ? P::accent.withAlpha (0.7f) : P::panelEdge.brighter (0.3f));
        g.drawEllipse (led.reduced (0.5f), 1.0f);
    }
}
