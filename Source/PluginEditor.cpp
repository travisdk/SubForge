// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.h"

namespace
{
constexpr float kMinScale = 0.75f, kMaxScale = 2.0f;

// Hosts scale plug-ins differently (Ardour applies its own UI scale on Linux, Reaper does not),
// so the chosen size is remembered per host. Older versions kept one shared "uiScale".
juce::String uiScaleKey()
{
    juce::String host = juce::PluginHostType().getHostDescription();
    if (host == "Unknown")
        host = juce::File::getSpecialLocation (juce::File::hostApplicationPath).getFileNameWithoutExtension();
    return "uiScale." + host.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}

// The largest scale whose window still fits the screen the editor is on (or the main one),
// taking the host's own scaling of the editor into account.
float largestFittingScale (const juce::Component& c, int designHeight)
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* display = c.isShowing() ? displays.getDisplayForRect (c.getScreenBounds()) : displays.getPrimaryDisplay();
    if (display == nullptr)
        return kMaxScale;
    const auto hostScale = juce::jmax (0.01f, std::sqrt (std::abs (c.getTransform().getDeterminant())));
    const auto area = display->userBounds.reduced (20.0f, 40.0f); // leave room for title bars and panels
    return juce::jmax (kMinScale, juce::jmin (area.getWidth() / (float) MainView::kWidth,
                                              area.getHeight() / (float) designHeight) / hostScale);
}
} // namespace

SubForgeEditor::SubForgeEditor (SubForgeProcessor& p)
    : AudioProcessorEditor (&p), processor (p), scaleKey (uiScaleKey())
{
    setLookAndFeel (&lookAndFeel);
    lookAndFeel.setAccent (juce::Colour::fromString (
        processor.getSettings().getValue ("accentColour", SubForgeLookAndFeel::accentChoices().front().colour.toString())));

    view = std::make_unique<MainView> (processor, lookAndFeel, [this] (float s) { setUiScale (s); },
                                       [this] { return currentScale(); },
                                       [this] { applyLayout (preferredScale); });
    addAndMakeVisible (*view);

    auto& settings = processor.getSettings();
    preferredScale = juce::jlimit (kMinScale, kMaxScale,
                                   (float) settings.getDoubleValue (scaleKey, settings.getDoubleValue ("uiScale", 1.0)));
    setConstrainer (&constrainer);
    setResizable (true, true);
    applyLayout (preferredScale);
}

SubForgeEditor::~SubForgeEditor()
{
    // remember a size chosen with the resize corner as well
    processor.getSettings().setValue (scaleKey, preferredScale);
    processor.getSettings().saveIfNeeded();
    view.reset();
    setLookAndFeel (nullptr);
}

void SubForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (SubForgeLookAndFeel::Palette::background);
}

void SubForgeEditor::Constrainer::checkBounds (juce::Rectangle<int>& bounds, const juce::Rectangle<int>& previous,
                                              const juce::Rectangle<int>& limits, bool top, bool left, bool bottom, bool right)
{
    const auto ratio = getFixedAspectRatio();
    if (! (top || left || bottom || right)) // not the resize corner
        setFixedAspectRatio (0.0);
    ComponentBoundsConstrainer::checkBounds (bounds, previous, limits, top, left, bottom, right);
    setFixedAspectRatio (ratio);
}

// The largest scale at which the whole view fits the editor.
float SubForgeEditor::currentScale() const
{
    return juce::jmin ((float) getWidth() / (float) MainView::kWidth, (float) getHeight() / (float) view->designHeight());
}

void SubForgeEditor::resized()
{
    // only a drag of the resize corner is the user's choice; other resizes come from the host
    if (! applyingLayout && resizableCorner != nullptr && resizableCorner->isMouseButtonDown())
        preferredScale = currentScale();

    // vector drawing throughout, so a transform keeps everything sharp at any scale;
    // a host-set size with another aspect ratio leaves the view centred with margins
    const auto scale = currentScale();
    view->setTransform (juce::AffineTransform::scale (scale).translated (
        ((float) getWidth() - (float) MainView::kWidth * scale) * 0.5f,
        ((float) getHeight() - (float) view->designHeight() * scale) * 0.5f));
    view->setBounds (0, 0, MainView::kWidth, view->designHeight());
}

void SubForgeEditor::setScaleFactor (float newScale)
{
    AudioProcessorEditor::setScaleFactor (newScale);
    applyLayout (preferredScale); // the host's scale changes what fits on screen
}

void SubForgeEditor::applyLayout (float scale)
{
    const juce::ScopedValueSetter<bool> guard (applyingLayout, true);
    const int height = view->designHeight();
    scale = juce::jlimit (kMinScale, juce::jmin (kMaxScale, largestFittingScale (*this, height)), scale);
    // set on the constrainer directly: setResizeLimits() would first re-fit the current size
    // to the new limits with the old aspect ratio and send the host an extra, wrong-sized resize
    constrainer.setSizeLimits (juce::roundToInt (MainView::kWidth * kMinScale), juce::roundToInt ((float) height * kMinScale),
                               juce::roundToInt (MainView::kWidth * kMaxScale), juce::roundToInt ((float) height * kMaxScale));
    constrainer.setFixedAspectRatio ((double) MainView::kWidth / (double) height);
    setSize (juce::roundToInt (MainView::kWidth * scale), juce::roundToInt ((float) height * scale));
}

void SubForgeEditor::setUiScale (float scale)
{
    preferredScale = scale;
    applyLayout (scale);
    processor.getSettings().setValue (scaleKey, preferredScale);
    processor.getSettings().saveIfNeeded();
}
