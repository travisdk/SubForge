#include "PluginEditor.h"

namespace
{
const juce::Colour background { 0xff1b1d22 };
const juce::Colour panel { 0xff262931 };
const juce::Colour outline { 0xff3a3e48 };
const juce::Colour accent { 0xffff8a3d };
const juce::Colour text { 0xffe6e6e6 };
const juce::Colour dimText { 0xff9aa0aa };

constexpr int kHeaderHeight = 52;
constexpr int kMargin = 12;
} // namespace

SubForgeEditor::SubForgeEditor (SubForgeProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    lookAndFeel.setColour (juce::Slider::rotarySliderFillColourId, accent);
    lookAndFeel.setColour (juce::Slider::rotarySliderOutlineColourId, outline);
    lookAndFeel.setColour (juce::Slider::thumbColourId, accent);
    lookAndFeel.setColour (juce::Slider::textBoxTextColourId, text);
    lookAndFeel.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour (juce::ComboBox::backgroundColourId, outline);
    lookAndFeel.setColour (juce::ComboBox::outlineColourId, outline);
    lookAndFeel.setColour (juce::ComboBox::textColourId, text);
    lookAndFeel.setColour (juce::ComboBox::arrowColourId, accent);
    lookAndFeel.setColour (juce::PopupMenu::backgroundColourId, panel);
    lookAndFeel.setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.35f));
    lookAndFeel.setColour (juce::TextButton::buttonColourId, outline);
    lookAndFeel.setColour (juce::TextButton::textColourOffId, text);
    lookAndFeel.setColour (juce::Label::textColourId, dimText);
    setLookAndFeel (&lookAndFeel);

    sections = {
        { "OSCILLATOR", { addChoice ("wave", "Wave"), addKnob ("octave", "Octave"), addKnob ("detune", "Detune"),
                          addKnob ("sub", "Sub"), addChoice ("subWave", "Sub Wave") }, {} },
        { "FILTER", { addKnob ("cutoff", "Cutoff"), addKnob ("resonance", "Resonance"), addKnob ("envAmount", "Env Amt"),
                      addKnob ("filterDecay", "F. Decay"), addKnob ("drive", "Drive") }, {} },
        { "AMP", { addKnob ("attack", "Attack"), addKnob ("decay", "Decay"), addKnob ("sustain", "Sustain"),
                   addKnob ("release", "Release") }, {} },
        { "PLAY", { addKnob ("glide", "Glide"), addKnob ("velocity", "Velocity"), addKnob ("volume", "Volume") }, {} },
    };

    for (int i = 0; i < subforge::kNumPresets; ++i)
        presetBox.addItem (subforge::kPresets[i].name, i + 1);
    presetBox.onChange = [this]
    {
        if (const int id = presetBox.getSelectedId(); id > 0)
            processor.loadPreset (id - 1);
    };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    randomButton.onClick = [this] { processor.randomizePatch(); };
    mutateButton.onClick = [this] { processor.mutatePatch(); };
    randomButton.setColour (juce::TextButton::buttonColourId, accent.darker (0.3f));

    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &prevButton, &nextButton, &randomButton, &mutateButton })
        addAndMakeVisible (c);

    processor.addChangeListener (this);
    refreshPresetBox();
    setSize (880, 470);
}

SubForgeEditor::~SubForgeEditor()
{
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

SubForgeEditor::Control* SubForgeEditor::addKnob (const char* paramId, const juce::String& labelText)
{
    auto control = std::make_unique<Control>();
    control->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    control->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
    control->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    control->sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, control->slider);
    control->label.setText (labelText, juce::dontSendNotification);
    control->label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (control->slider);
    addAndMakeVisible (control->label);
    return controls.emplace_back (std::move (control)).get();
}

SubForgeEditor::Control* SubForgeEditor::addChoice (const char* paramId, const juce::String& labelText)
{
    auto control = std::make_unique<Control>();
    control->isChoice = true;
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
        control->box.addItemList (choice->choices, 1);
    control->boxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, paramId, control->box);
    control->label.setText (labelText, juce::dontSendNotification);
    control->label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (control->box);
    addAndMakeVisible (control->label);
    return controls.emplace_back (std::move (control)).get();
}

void SubForgeEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetBox();
}

void SubForgeEditor::refreshPresetBox()
{
    const auto name = processor.getPatchName();
    const int index = processor.getCurrentProgram();

    if (name == subforge::kPresets[index].name)
        presetBox.setSelectedId (index + 1, juce::dontSendNotification);
    else
        presetBox.setText (name, juce::dontSendNotification);
}

void SubForgeEditor::stepPreset (int delta)
{
    const int n = subforge::kNumPresets;
    processor.loadPreset ((processor.getCurrentProgram() + delta + n) % n);
}

void SubForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    g.setColour (accent);
    g.setFont (juce::Font (24.0f, juce::Font::bold));
    g.drawText ("SubForge", headerArea.withTrimmedLeft (kMargin), juce::Justification::centredLeft);

    for (const auto& section : sections)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (section.bounds.toFloat(), 8.0f);
        g.setColour (dimText);
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (section.title, section.bounds.withHeight (26).withTrimmedLeft (10),
                    juce::Justification::centredLeft);
    }
}

void SubForgeEditor::layoutSection (Section& section, juce::Rectangle<int> area)
{
    section.bounds = area;
    auto content = area.reduced (6).withTrimmedTop (22);
    const int cellWidth = content.getWidth() / (int) section.controls.size();

    for (auto* control : section.controls)
    {
        auto cell = content.removeFromLeft (cellWidth).reduced (4, 0);
        control->label.setBounds (cell.removeFromTop (20));

        if (control->isChoice)
            control->box.setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 26).translated (0, -10));
        else
            control->slider.setBounds (cell);
    }
}

void SubForgeEditor::resized()
{
    auto area = getLocalBounds();
    headerArea = area.removeFromTop (kHeaderHeight);

    auto header = headerArea.reduced (kMargin, 12);
    mutateButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (6);
    randomButton.setBounds (header.removeFromRight (100));
    header.removeFromRight (16);
    nextButton.setBounds (header.removeFromRight (28));
    presetBox.setBounds (header.removeFromRight (220));
    prevButton.setBounds (header.removeFromRight (28));

    area.reduce (kMargin, 0);
    area.removeFromBottom (kMargin);
    const int rowHeight = (area.getHeight() - kMargin) / 2;
    auto topRow = area.removeFromTop (rowHeight);
    area.removeFromTop (kMargin);
    auto bottomRow = area;

    const int half = (topRow.getWidth() - kMargin) / 2;
    layoutSection (sections[0], topRow.removeFromLeft (half));
    topRow.removeFromLeft (kMargin);
    layoutSection (sections[1], topRow);

    const int ampWidth = (bottomRow.getWidth() - kMargin) * 4 / 7;
    layoutSection (sections[2], bottomRow.removeFromLeft (ampWidth));
    bottomRow.removeFromLeft (kMargin);
    layoutSection (sections[3], bottomRow);
}
