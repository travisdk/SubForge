#include "PluginEditor.h"

namespace
{
constexpr float kMinScale = 0.75f, kMaxScale = 2.0f;
}

SubForgeEditor::SubForgeEditor (SubForgeProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    lookAndFeel.setAccent (juce::Colour::fromString (
        processor.getSettings().getValue ("accentColour", SubForgeLookAndFeel::accentChoices().front().colour.toString())));

    view = std::make_unique<MainView> (processor, lookAndFeel, [this] (float s) { setUiScale (s); },
                                       [this] { return currentScale(); });
    addAndMakeVisible (*view);

    setResizable (true, true);
    setResizeLimits (juce::roundToInt (MainView::kWidth * kMinScale), juce::roundToInt (MainView::kHeight * kMinScale),
                     juce::roundToInt (MainView::kWidth * kMaxScale), juce::roundToInt (MainView::kHeight * kMaxScale));
    if (auto* aspect = getConstrainer())
        aspect->setFixedAspectRatio ((double) MainView::kWidth / (double) MainView::kHeight);

    const float saved = (float) processor.getSettings().getDoubleValue ("uiScale", 1.0);
    const float scale = juce::jlimit (kMinScale, kMaxScale, saved);
    setSize (juce::roundToInt (MainView::kWidth * scale), juce::roundToInt (MainView::kHeight * scale));
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
    view->setBounds (0, 0, MainView::kWidth, MainView::kHeight);
}

void SubForgeEditor::setUiScale (float scale)
{
    scale = juce::jlimit (kMinScale, kMaxScale, scale);
    setSize (juce::roundToInt (MainView::kWidth * scale), juce::roundToInt (MainView::kHeight * scale));
    processor.getSettings().setValue ("uiScale", scale);
    processor.getSettings().saveIfNeeded();
}
