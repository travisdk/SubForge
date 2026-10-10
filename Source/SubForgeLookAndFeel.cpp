#include "SubForgeLookAndFeel.h"
#include "BinaryData.h"

using P = SubForgeLookAndFeel::Palette;

SubForgeLookAndFeel::SubForgeLookAndFeel()
{
    labelFace = juce::Typeface::createSystemTypefaceFor (BinaryData::ChakraPetchSemiBold_ttf, BinaryData::ChakraPetchSemiBold_ttfSize);
    titleFace = juce::Typeface::createSystemTypefaceFor (BinaryData::ChakraPetchBold_ttf, BinaryData::ChakraPetchBold_ttfSize);
    valueFace = juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMonoMedium_ttf, BinaryData::JetBrainsMonoMedium_ttfSize);

    setColour (juce::ResizableWindow::backgroundColourId, P::background);
    setColour (juce::Label::textColourId, P::dimText);
    setColour (juce::Slider::textBoxTextColourId, P::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, P::field);
    setColour (juce::TextEditor::textColourId, P::text);
    setColour (juce::CaretComponent::caretColourId, P::text);
    setColour (juce::ComboBox::textColourId, P::text);
    setColour (juce::PopupMenu::textColourId, P::text);
    setColour (juce::PopupMenu::highlightedTextColourId, P::text);
    setColour (juce::TextButton::buttonColourId, P::field);
    setColour (juce::TextButton::textColourOffId, P::text);
    setColour (juce::TextButton::textColourOnId, P::background);
    setColour (juce::TextEditor::outlineColourId, P::panelEdge);
    setColour (juce::AlertWindow::backgroundColourId, P::panel);
    setColour (juce::AlertWindow::textColourId, P::text);
    setColour (juce::AlertWindow::outlineColourId, P::panelEdge);
    setAccent (accentChoices().front().colour);
}

const std::vector<SubForgeLookAndFeel::AccentChoice>& SubForgeLookAndFeel::accentChoices()
{
    static const std::vector<AccentChoice> choices {
        { "Lime", juce::Colour (0xffc8ff2e) },   { "Orange", juce::Colour (0xffff9f43) },
        { "Amber", juce::Colour (0xffffc23d) },  { "Mint", juce::Colour (0xff3ddca0) },
        { "Cyan", juce::Colour (0xff3dc8ff) },   { "Blue", juce::Colour (0xff5b8cff) },
        { "Violet", juce::Colour (0xffa77bff) }, { "Pink", juce::Colour (0xffff5fa2) },
        { "Red", juce::Colour (0xffff5252) },
    };
    return choices;
}

void SubForgeLookAndFeel::setAccent (juce::Colour newAccent)
{
    accent = newAccent;
    setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.4f));
    setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.4f));
    setColour (juce::TextEditor::focusedOutlineColourId, accent);
}

juce::Colour SubForgeLookAndFeel::accentFor (const juce::Component& c) const
{
    return (bool) c.getProperties().getWithDefault (secondaryProperty, false) ? P::secondary : accent;
}

void SubForgeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                            float startAngle, float endAngle, juce::Slider& slider)
{
    const int maxSize = slider.getProperties().getWithDefault (knobSizeProperty, kKnobSize);
    const float size = (float) juce::jmin (width, height, maxSize);
    const auto centre = juce::Rectangle<int> (x, y, width, height).toFloat().getCentre();
    const float thickness = juce::jmax (3.0f, size * 0.09f);
    const float ringRadius = size * 0.5f - thickness * 0.5f - 1.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const bool hot = slider.isEnabled() && slider.isMouseOverOrDragging();
    const auto colour = accentFor (slider);
    const juce::PathStrokeType arcStroke (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (P::track);
    g.strokePath (track, arcStroke);

    // bipolar ranges (pitch env, volume, mod amounts) fill from their zero point
    float fromAngle = startAngle;
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
        fromAngle = startAngle + (float) slider.valueToProportionOfLength (0.0) * (endAngle - startAngle);

    if (std::abs (angle - fromAngle) > 0.01f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                             juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
        g.setColour (hot ? colour.brighter (0.2f) : colour);
        g.strokePath (value, arcStroke);
    }

    const float bodyRadius = ringRadius - thickness * 1.1f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);
    g.setColour (P::knobBody);
    g.fillEllipse (body);
    g.setColour (hot ? P::panelEdge.brighter (0.4f) : P::panelEdge);
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    juce::Path pointer;
    pointer.startNewSubPath (centre.getPointOnCircumference (bodyRadius * 0.35f, angle));
    pointer.lineTo (centre.getPointOnCircumference (bodyRadius * 0.88f, angle));
    g.setColour (P::text);
    g.strokePath (pointer, juce::PathStrokeType (juce::jmax (2.0f, size * 0.045f), juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

juce::Label* SubForgeLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (valueFont ((float) slider.getProperties().getWithDefault (valueSizeProperty, 13.0f)));
    label->setMinimumHorizontalScale (0.5f); // squeeze long values rather than cut them off
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
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (hot ? accentFor (box).withAlpha (0.7f) : P::panelEdge);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    const float cx = (float) width - 12.0f, cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
    chevron.lineTo (cx, cy + 2.0f);
    chevron.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (P::dimText);
    g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SubForgeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 26, box.getHeight() - 2);
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
        g.setColour (accent.withAlpha (0.18f));
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
    }

    const juce::Point<float> marker { (float) r.getX() + 10.0f, (float) r.getCentreY() };
    if (textColour != nullptr)
    {
        // colour swatch item, ringed when it is the current choice
        g.setColour (*textColour);
        g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (marker));
        if (isTicked)
        {
            g.setColour (P::text);
            g.drawEllipse (juce::Rectangle<float> (15.0f, 15.0f).withCentre (marker), 1.5f);
        }
    }
    else if (isTicked)
    {
        g.setColour (accent);
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (marker));
    }

    g.setColour (isActive ? (isTicked && textColour == nullptr ? accent : P::text) : P::dimText);
    g.setFont (getPopupMenuFont());
    g.drawFittedText (text, r.withTrimmedLeft (20).withTrimmedRight (6), juce::Justification::centredLeft, 1);
}

void SubForgeLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area,
                                                     const juce::String& sectionName)
{
    g.setColour (P::dimText);
    g.setFont (titleFont (11.0f).withExtraKerningFactor (0.14f));
    g.drawText (sectionName.toUpperCase(), area.reduced (10, 0), juce::Justification::bottomLeft);
}

// Buttons are flat chips. A toggled-on button is filled with its accent;
// otherwise it is outlined on the dark field colour.
void SubForgeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour, bool isHighlighted, bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool lit = button.getToggleState();
    const bool outlined = button.getProperties().getWithDefault (outlineProperty, false);
    auto fill = lit ? accentFor (button) : backgroundColour;
    if (isDown)
        fill = fill.darker (0.2f);
    else if (isHighlighted)
        fill = fill.brighter (0.08f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 6.0f);
    if (outlined && ! lit)
    {
        g.setColour (accentFor (button));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.5f);
    }
    else if (! lit)
    {
        g.setColour (isHighlighted ? P::panelEdge.brighter (0.5f) : P::panelEdge.brighter (0.15f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
    }
}

void SubForgeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const bool lit = button.getToggleState();
    const bool outlined = button.getProperties().getWithDefault (outlineProperty, false);
    g.setFont (getTextButtonFont (button, button.getHeight()).withExtraKerningFactor (0.08f));
    g.setColour (lit ? P::background
                 : outlined ? accentFor (button)
                     : button.findColour (juce::TextButton::textColourOffId).withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 0), juce::Justification::centred, 1);
}

// Toggles are round LEDs: a ring that glows in the accent colour when on.
void SubForgeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool isHighlighted, bool)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 2.0f;
    const auto ring = juce::Rectangle<float> (d, d).withCentre (bounds.getCentre());
    const auto colour = accentFor (button);

    g.setColour (button.getToggleState() ? colour : (isHighlighted ? P::panelEdge.brighter (0.6f) : P::panelEdge.brighter (0.3f)));
    g.drawEllipse (ring.reduced (1.0f), 2.0f);

    const auto core = ring.reduced (d * 0.27f);
    if (button.getToggleState())
    {
        g.setColour (colour.withAlpha (0.25f));
        g.fillEllipse (core.expanded (3.0f));
        g.setColour (colour);
        g.fillEllipse (core);
    }
    else
    {
        g.setColour (P::field);
        g.fillEllipse (core);
    }
}
