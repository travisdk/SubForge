// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#include "AboutOverlay.h"
#include "SubForgeVersion.h"
#include "BinaryData.h"

namespace
{
using P = SubForgeLookAndFeel::Palette;
constexpr int kAboutWidth = 460, kAboutHeight = 342, kLicencesWidth = 700, kLicencesHeight = 380;
constexpr int kMargin = 28, kTop = 22;
const juce::Colour kPopup { 0xff181c1a }; // a little lighter than the panels, so the panel stands out from the editor
} // namespace

AboutOverlay::AboutOverlay (SubForgeLookAndFeel& lf, std::function<void()> onClose)
    : lnf (lf), close (std::move (onClose))
{
    const auto link = [this] (juce::HyperlinkButton& button, const char* text, const char* url)
    {
        button.setButtonText (text);
        button.setURL (juce::URL (url));
        button.setColour (juce::HyperlinkButton::textColourId, lnf.getAccent());
        button.setTooltip (url);
        button.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (button);
    };
    link (homepage, "music.henrikjuhl.dk", "https://music.henrikjuhl.dk");
    link (project, "music.henrikjuhl.dk/plugins/subforge", "https://music.henrikjuhl.dk/plugins/subforge/");

    licencesButton.setTooltip ("Licences and third-party notices.");
    licencesButton.onClick = [this] { setShowingLicences (! showingLicences); };
    addAndMakeVisible (licencesButton);

    notices.setMultiLine (true, true);
    notices.setReadOnly (true);
    notices.setScrollbarsShown (true);
    notices.setCaretVisible (false);
    notices.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain)));
    notices.setIndents (12, 10);
    notices.setColour (juce::TextEditor::backgroundColourId, P::field);
    notices.setColour (juce::TextEditor::textColourId, P::text);
    notices.setColour (juce::TextEditor::outlineColourId, P::panelEdge);
    notices.setColour (juce::TextEditor::focusedOutlineColourId, P::panelEdge);
    notices.setText (juce::String::fromUTF8 (BinaryData::THIRDPARTYNOTICES_txt, BinaryData::THIRDPARTYNOTICES_txtSize),
                     juce::dontSendNotification);
    notices.moveCaretToTop (false);
    notices.onEscapeKey = [this] { close(); }; // the text box has focus and would swallow Escape
    addChildComponent (notices);

    setWantsKeyboardFocus (true);
    setShowingLicences (false);
}

void AboutOverlay::focusText()
{
    // so Escape works without clicking first (hosts may still keep the keys)
    if (showingLicences)
        notices.grabKeyboardFocus();
    else
        grabKeyboardFocus();
}

void AboutOverlay::setShowingLicences (bool show)
{
    showingLicences = show;
    licencesButton.setButtonText (show ? "Back" : "Licences");
    notices.setVisible (show);
    homepage.setVisible (! show);
    project.setVisible (! show);
    if (show)
        notices.moveCaretToTop (false);
    resized();
    repaint();
    if (isShowing())
        focusText();
}

juce::Rectangle<int> AboutOverlay::panelBounds() const
{
    const int w = showingLicences ? kLicencesWidth : kAboutWidth, h = showingLicences ? kLicencesHeight : kAboutHeight;
    return getLocalBounds().withSizeKeepingCentre (juce::jmin (w, getWidth() - 20), juce::jmin (h, getHeight() - 20));
}

juce::Rectangle<int> AboutOverlay::crossArea() const
{
    const auto panel = panelBounds();
    return { panel.getRight() - 44, panel.getY() + 8, 36, 36 };
}

void AboutOverlay::paint (juce::Graphics& g)
{
    const auto panel = panelBounds();
    const auto r = panel.toFloat().reduced (1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 14.0f);
    g.setColour (kPopup);
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (lnf.getAccent().withAlpha (0.8f));
    g.drawRoundedRectangle (r, 14.0f, 1.5f);

    // the cross
    g.setColour (P::dimText.brighter (0.3f));
    const auto c = crossArea().toFloat().getCentre();
    g.drawLine (c.x - 6.0f, c.y - 6.0f, c.x + 6.0f, c.y + 6.0f, 2.2f);
    g.drawLine (c.x - 6.0f, c.y + 6.0f, c.x + 6.0f, c.y - 6.0f, 2.2f);

    auto area = panel.reduced (kMargin, kTop);
    const auto line = [&area] (int height) { return area.removeFromTop (height); };

    if (showingLicences)
    {
        g.setColour (P::text);
        g.setFont (lnf.titleFont (22.0f));
        g.drawText ("Licences", line (32), juce::Justification::centredLeft);
        return;
    }

    g.setColour (lnf.getAccent());
    g.setFont (lnf.titleFont (30.0f).withExtraKerningFactor (0.08f));
    g.drawText ("SUBFORGE", line (38), juce::Justification::centredLeft);

    g.setColour (P::dimText.brighter (0.2f));
    g.setFont (lnf.labelFont (15.0f));
    g.drawText (juce::String ("Version ") + SUBFORGE_VERSION_STRING, line (22), juce::Justification::centredLeft); // the exact build, e.g. 1.4.0
    g.setColour (P::text);
    g.drawText ("Mono bass synth with a step sequencer", line (24), juce::Justification::centredLeft);
    line (6);
    g.drawText ("by Henrik Juhl", line (22), juce::Justification::centredLeft);
    g.drawText ("Free software under the GNU AGPLv3, with no warranty", line (22), juce::Justification::centredLeft);
    line (8 + 2 * 24 + 8); // the two links lie here (see resized)

    g.setColour (P::dimText.brighter (0.2f));
    g.setFont (lnf.labelFont (14.0f));
    g.drawFittedText ("Made with JUCE and clap-juce-extensions. Font: Chakra Petch (SIL OFL).", line (40), juce::Justification::topLeft, 2);
    line (4);
    g.drawText ("Click Licences for all third-party notices.", line (20).withTrimmedRight (110), juce::Justification::centredLeft);
}

void AboutOverlay::resized()
{
    const auto panel = panelBounds();
    if (showingLicences)
    {
        licencesButton.setBounds (panel.getRight() - 44 - 80, panel.getY() + 10, 80, 28); // Back, left of the cross
        notices.setBounds (panel.withTrimmedTop (58).reduced (kMargin, 0).withTrimmedBottom (kTop));
        return;
    }
    licencesButton.setBounds (panel.getRight() - kMargin - 96, panel.getBottom() - kTop - 28, 96, 28);
    // the links: under the licence line
    const int top = panel.getY() + kTop + 38 + 22 + 24 + 6 + 22 + 22 + 8;
    homepage.setBounds (panel.getX() + kMargin, top, panel.getWidth() - 2 * kMargin, 24);
    project.setBounds (panel.getX() + kMargin, top + 24, panel.getWidth() - 2 * kMargin, 24);
}

void AboutOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (crossArea().contains (e.getPosition()) || ! panelBounds().contains (e.getPosition()))
        close();
}

bool AboutOverlay::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        close();
        return true;
    }
    return false;
}
