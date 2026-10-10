#include "PluginEditor.h"

namespace
{
constexpr float kMinScale = 0.75f, kMaxScale = 2.0f;

// The largest scale whose window still fits the screen the editor is on (or the main one).
float largestFittingScale (const juce::Component& c, int designHeight)
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* display = c.isShowing() ? displays.getDisplayForRect (c.getScreenBounds()) : displays.getPrimaryDisplay();
    if (display == nullptr)
        return kMaxScale;
    const auto area = display->userBounds.reduced (20.0f, 40.0f); // leave room for title bars and panels
    return juce::jmax (kMinScale, juce::jmin (area.getWidth() / (float) MainView::kWidth, area.getHeight() / (float) designHeight));
}
} // namespace

SubForgeEditor::SubForgeEditor (SubForgeProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    lookAndFeel.setAccent (juce::Colour::fromString (
        processor.getSettings().getValue ("accentColour", SubForgeLookAndFeel::accentChoices().front().colour.toString())));

    view = std::make_unique<MainView> (processor, lookAndFeel, [this] (float s) { setUiScale (s); },
                                       [this] { return currentScale(); },
                                       [this] { applyLayout (currentScale()); });
    addAndMakeVisible (*view);

    setResizable (true, true);
    applyLayout ((float) processor.getSettings().getDoubleValue ("uiScale", 1.0));
}

SubForgeEditor::~SubForgeEditor()
{
    // remember a size chosen with the resize corner as well
    processor.getSettings().setValue ("uiScale", currentScale());
    processor.getSettings().saveIfNeeded();
    view.reset();
    setLookAndFeel (nullptr);
}

void SubForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (SubForgeLookAndFeel::Palette::background);
}

void SubForgeEditor::resized()
{
    // vector drawing throughout, so a transform keeps everything sharp at any scale
    view->setTransform (juce::AffineTransform::scale (currentScale()));
    view->setBounds (0, 0, MainView::kWidth, view->designHeight());
}

void SubForgeEditor::applyLayout (float scale)
{
    const int height = view->designHeight();
    scale = juce::jlimit (kMinScale, juce::jmin (kMaxScale, largestFittingScale (*this, height)), scale);
    setResizeLimits (juce::roundToInt (MainView::kWidth * kMinScale), juce::roundToInt ((float) height * kMinScale),
                     juce::roundToInt (MainView::kWidth * kMaxScale), juce::roundToInt ((float) height * kMaxScale));
    if (auto* aspect = getConstrainer())
        aspect->setFixedAspectRatio ((double) MainView::kWidth / (double) height);
    setSize (juce::roundToInt (MainView::kWidth * scale), juce::roundToInt ((float) height * scale));
}

void SubForgeEditor::setUiScale (float scale)
{
    applyLayout (scale);
    processor.getSettings().setValue ("uiScale", currentScale());
    processor.getSettings().saveIfNeeded();
}
