#include "AboutOverlay.h"
#include "BinaryData.h"

namespace
{
using P = SubForgeLookAndFeel::Palette;
constexpr int kPanelWidth = 640, kPanelHeight = 420, kHeaderHeight = 96;
} // namespace

AboutOverlay::AboutOverlay (SubForgeLookAndFeel& lf, std::function<void()> onClose)
    : lnf (lf), close (std::move (onClose))
{
    notices.setMultiLine (true, true);
    notices.setReadOnly (true);
    notices.setScrollbarsShown (true);
    notices.setCaretVisible (false);
    notices.setFont (lnf.regularFont (12.0f));
    notices.setColour (juce::TextEditor::backgroundColourId, P::field);
    notices.setColour (juce::TextEditor::outlineColourId, P::panelEdge);
    notices.setColour (juce::TextEditor::focusedOutlineColourId, P::panelEdge);
    notices.setText (juce::String::fromUTF8 (BinaryData::THIRDPARTYNOTICES_txt, BinaryData::THIRDPARTYNOTICES_txtSize),
                     juce::dontSendNotification);
    notices.onEscapeKey = [this] { close(); }; // the text box has focus and would swallow Escape
    addAndMakeVisible (notices);

    closeButton.onClick = [this] { close(); };
    addAndMakeVisible (closeButton);

    setWantsKeyboardFocus (true);
}

juce::Rectangle<int> AboutOverlay::panelBounds() const
{
    return getLocalBounds().withSizeKeepingCentre (juce::jmin (kPanelWidth, getWidth() - 40),
                                                   juce::jmin (kPanelHeight, getHeight() - 40));
}

void AboutOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.55f));

    const auto panel = panelBounds();
    g.setColour (P::panel);
    g.fillRoundedRectangle (panel.toFloat(), 8.0f);
    g.setColour (P::panelEdge.brighter (0.2f));
    g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), 8.0f, 1.0f);

    auto header = panel.reduced (20, 16).withHeight (kHeaderHeight - 16);
    g.setColour (lnf.getAccent());
    g.setFont (lnf.boldFont (22.0f).withExtraKerningFactor (0.08f));
    g.drawText ("SUBFORGE", header.removeFromTop (28), juce::Justification::centredLeft);

    g.setColour (P::text);
    g.setFont (lnf.regularFont (13.0f));
    g.drawText (juce::String::fromUTF8 ("Mono bass synth  \xc2\xb7  version ") + JucePlugin_VersionString,
                header.removeFromTop (20), juce::Justification::centredLeft);
    g.setColour (P::dimText);
    g.drawText (juce::String::fromUTF8 ("\xc2\xa9 2026 Henrik Juhl  \xc2\xb7  Built with JUCE, used under a commercial JUCE licence."),
                header.removeFromTop (20), juce::Justification::centredLeft);
}

void AboutOverlay::resized()
{
    auto panel = panelBounds().reduced (20, 16);
    panel.removeFromTop (kHeaderHeight - 16);
    closeButton.setBounds (panel.removeFromBottom (28).removeFromRight (90));
    panel.removeFromBottom (10);
    notices.setBounds (panel);
}

void AboutOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! panelBounds().contains (e.getPosition()))
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
