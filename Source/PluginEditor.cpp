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
    lookAndFeel.setColour (juce::ToggleButton::tickColourId, accent);
    lookAndFeel.setColour (juce::ToggleButton::tickDisabledColourId, dimText);
    lookAndFeel.setColour (juce::Label::textColourId, dimText);
    setLookAndFeel (&lookAndFeel);

    lfoRateControl = nullptr;
    sections = {
        { "OSCILLATOR", { choice ("wave", "Wave"), knob ("octave", "Octave"), knob ("detune", "Detune"),
                          knob ("sub", "Sub"), choice ("subWave", "Sub Wave") } },
        { "FILTER", { choice ("filterMode", "Mode"), knob ("cutoff", "Cutoff"), knob ("resonance", "Resonance"),
                      knob ("envAmount", "Env Amt"), knob ("filterDecay", "F. Decay"), knob ("drive", "Drive"),
                      knob ("keyTrack", "Key Track") } },
        { "AMP ENV", { knob ("attack", "Attack"), knob ("decay", "Decay"), knob ("sustain", "Sustain"),
                   knob ("release", "Release") } },
        { "PITCH & CLICK", { knob ("pitchEnv", "Pitch Env"), knob ("pitchDecay", "P. Decay"), knob ("click", "Click") } },
        { "DISTORTION", { choice ("distType", "Type"), knob ("distDrive", "Gain"), knob ("distTone", "Tone"),
                          knob ("distMix", "Mix") } },
        { "LFO > CUTOFF", { choice ("lfoShape", "Shape"), choice ("lfoSync", "Sync"),
                            lfoRateControl = knob ("lfoRate", "Rate"), knob ("lfoDepth", "Depth"),
                            toggle ("lfoRetrig", "Retrig") } },
        { "PLAY", { knob ("glide", "Glide"), knob ("velocity", "Velocity"), knob ("accent", "Accent"),
                    knob ("volume", "Volume") } },
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
    timerCallback();
    startTimerHz (10);
    setSize (1000, 660);
}

SubForgeEditor::~SubForgeEditor()
{
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

SubForgeEditor::Control* SubForgeEditor::addControl (Kind kind, const char* paramId, const juce::String& labelText)
{
    using APVTS = juce::AudioProcessorValueTreeState;
    auto control = std::make_unique<Control>();
    control->kind = kind;

    switch (kind)
    {
        case Kind::Knob:
            control->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            control->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
            control->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            control->sliderAttachment = std::make_unique<APVTS::SliderAttachment> (processor.apvts, paramId, control->slider);
            addAndMakeVisible (control->slider);
            break;
        case Kind::Choice:
            if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
                control->box.addItemList (param->choices, 1);
            control->boxAttachment = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, paramId, control->box);
            addAndMakeVisible (control->box);
            break;
        case Kind::Toggle:
            control->toggleAttachment = std::make_unique<APVTS::ButtonAttachment> (processor.apvts, paramId, control->toggle);
            addAndMakeVisible (control->toggle);
            break;
    }

    control->label.setText (labelText, juce::dontSendNotification);
    control->label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (control->label);
    return controls.emplace_back (std::move (control)).get();
}

void SubForgeEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshPresetBox();
}

void SubForgeEditor::timerCallback()
{
    // Rate only matters while the LFO runs free
    const bool freeRunning = processor.apvts.getRawParameterValue ("lfoSync")->load() < 0.5f;
    lfoRateControl->slider.setEnabled (freeRunning);
    lfoRateControl->slider.setAlpha (freeRunning ? 1.0f : 0.35f);
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
    const int contentWidth = content.getWidth();
    const int totalWeight = section.weight();

    for (auto* control : section.controls)
    {
        auto cell = content.removeFromLeft (contentWidth * control->weight() / totalWeight).reduced (4, 0);
        control->label.setBounds (cell.removeFromTop (20));

        switch (control->kind)
        {
            case Kind::Knob:
                control->slider.setBounds (cell);
                break;
            case Kind::Choice:
                control->box.setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 26).translated (0, -10));
                break;
            case Kind::Toggle:
                control->toggle.setBounds (cell.withSizeKeepingCentre (28, 28).translated (0, -10));
                break;
        }
    }
}

// Sections in a row share its width in proportion to their controls' weights.
void SubForgeEditor::layoutRow (std::initializer_list<Section*> row, juce::Rectangle<int> area)
{
    int totalWeight = 0;
    for (auto* section : row)
        totalWeight += section->weight();

    const int available = area.getWidth() - kMargin * ((int) row.size() - 1);
    int index = 0;
    for (auto* section : row)
    {
        const bool last = ++index == (int) row.size();
        const int width = last ? area.getWidth() : available * section->weight() / totalWeight;
        layoutSection (*section, area.removeFromLeft (width));
        area.removeFromLeft (kMargin);
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
    const int rowHeight = (area.getHeight() - 2 * kMargin) / 3;

    auto nextRow = [&]
    {
        auto row = area.removeFromTop (rowHeight);
        area.removeFromTop (kMargin);
        return row;
    };

    layoutRow ({ &sections[0], &sections[1] }, nextRow());
    layoutRow ({ &sections[2], &sections[3], &sections[4] }, nextRow());
    layoutRow ({ &sections[5], &sections[6] }, nextRow());
}
