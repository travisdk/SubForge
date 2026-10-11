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

void SubForgeEditor::resized()
{
    // a resize we did not make ourselves is the user dragging the corner
    if (! applyingLayout)
        preferredScale = currentScale();

    // vector drawing throughout, so a transform keeps everything sharp at any scale
    view->setTransform (juce::AffineTransform::scale (currentScale()));
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
    setResizeLimits (juce::roundToInt (MainView::kWidth * kMinScale), juce::roundToInt ((float) height * kMinScale),
                     juce::roundToInt (MainView::kWidth * kMaxScale), juce::roundToInt ((float) height * kMaxScale));
    if (auto* aspect = getConstrainer())
        aspect->setFixedAspectRatio ((double) MainView::kWidth / (double) height);
    setSize (juce::roundToInt (MainView::kWidth * scale), juce::roundToInt ((float) height * scale));
}

void SubForgeEditor::setUiScale (float scale)
{
    preferredScale = scale;
    applyLayout (scale);
    processor.getSettings().setValue (scaleKey, preferredScale);
    processor.getSettings().saveIfNeeded();
}
