#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::NormalisableRange<float> skewedRange (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> range (lo, hi);
    range.setSkewForCentre (centre);
    return range;
}

// Parameters without a unit are 0..1 amounts and display as percent.
std::unique_ptr<juce::AudioParameterFloat> floatParam (const char* id, const char* name,
                                                       juce::NormalisableRange<float> range,
                                                       float defaultValue, const char* unit = "")
{
    const juce::String unitText (unit);
    const bool isPercent = unitText.isEmpty();

    auto toText = [unitText, isPercent] (float value, int)
    {
        if (isPercent)
            return juce::String (juce::roundToInt (value * 100.0f)) + " %";
        const auto number = std::abs (value) < 10.0f ? juce::String (value, 1) : juce::String (juce::roundToInt (value));
        return number + " " + unitText;
    };
    auto fromText = [isPercent] (const juce::String& text)
    {
        const float value = text.retainCharacters ("0123456789.-").getFloatValue();
        return isPercent ? value / 100.0f : value;
    };

    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, defaultValue,
                                                        juce::AudioParameterFloatAttributes()
                                                            .withStringFromValueFunction (toText)
                                                            .withValueFromStringFunction (fromText));
}
} // namespace

SubForgeProcessor::SubForgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SubForgeProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "wave", 1 }, "Wave",
                                                              StringArray { "Saw", "Square", "Pulse" }, 0));
    params.push_back (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Octave", -2, 1, 0));
    params.push_back (floatParam ("detune", "Detune", { 0.0f, 50.0f }, 0.0f, "ct"));
    params.push_back (floatParam ("sub", "Sub Level", { 0.0f, 1.0f }, 0.5f));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "subWave", 1 }, "Sub Wave",
                                                              StringArray { "Sine", "Square" }, 0));

    params.push_back (floatParam ("cutoff", "Cutoff", skewedRange (20.0f, 18000.0f, 800.0f), 400.0f, "Hz"));
    params.push_back (floatParam ("resonance", "Resonance", { 0.0f, 1.0f }, 0.3f));
    params.push_back (floatParam ("envAmount", "Env Amount", { 0.0f, 1.0f }, 0.5f));
    params.push_back (floatParam ("filterDecay", "Filter Decay", skewedRange (5.0f, 3000.0f, 300.0f), 250.0f, "ms"));
    params.push_back (floatParam ("drive", "Drive", { 0.0f, 1.0f }, 0.2f));

    params.push_back (floatParam ("attack", "Attack", skewedRange (0.5f, 2000.0f, 50.0f), 1.0f, "ms"));
    params.push_back (floatParam ("decay", "Decay", skewedRange (5.0f, 5000.0f, 400.0f), 600.0f, "ms"));
    params.push_back (floatParam ("sustain", "Sustain", { 0.0f, 1.0f }, 0.7f));
    params.push_back (floatParam ("release", "Release", skewedRange (5.0f, 3000.0f, 200.0f), 60.0f, "ms"));

    params.push_back (floatParam ("glide", "Glide", skewedRange (0.0f, 1000.0f, 100.0f), 50.0f, "ms"));
    params.push_back (floatParam ("velocity", "Velocity Sens", { 0.0f, 1.0f }, 0.5f));
    params.push_back (floatParam ("volume", "Volume", { -36.0f, 6.0f }, -6.0f, "dB"));

    return { params.begin(), params.end() };
}

subforge::BassParams SubForgeProcessor::readParams() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    subforge::BassParams p;
    p.wave = (int) get ("wave");
    p.octave = (int) get ("octave");
    p.detuneCents = get ("detune");
    p.sub = get ("sub");
    p.subWave = (int) get ("subWave");
    p.cutoffHz = get ("cutoff");
    p.resonance = get ("resonance");
    p.envAmount = get ("envAmount");
    p.filterDecayMs = get ("filterDecay");
    p.drive = get ("drive");
    p.attackMs = get ("attack");
    p.decayMs = get ("decay");
    p.sustain = get ("sustain");
    p.releaseMs = get ("release");
    p.glideMs = get ("glide");
    p.velocitySens = get ("velocity");
    p.gain = juce::Decibels::decibelsToGain (get ("volume"));
    return p;
}

void SubForgeProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setParams (readParams());
    synth.prepare (sampleRate);
}

bool SubForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void SubForgeProcessor::handleMidi (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
        synth.noteOn (message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        synth.noteOff (message.getNoteNumber());
    else if (message.isAllNotesOff() || message.isAllSoundOff())
        synth.allNotesOff();
    else if (message.isPitchWheel())
        synth.setPitchBend ((float) (message.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
}

void SubForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    synth.setParams (readParams());

    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer (0);
    int position = 0;

    for (const auto metadata : midi)
    {
        const int eventPos = juce::jlimit (0, numSamples, metadata.samplePosition);
        if (eventPos > position)
        {
            synth.render (left + position, eventPos - position);
            position = eventPos;
        }
        handleMidi (metadata.getMessage());
    }

    if (position < numSamples)
        synth.render (left + position, numSamples - position);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

juce::AudioProcessorEditor* SubForgeProcessor::createEditor()
{
    return new SubForgeEditor (*this);
}

const juce::String SubForgeProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, subforge::kNumPresets) ? subforge::kPresets[index].name : "";
}

subforge::PatchValues SubForgeProcessor::currentPatch() const
{
    subforge::PatchValues values {};
    for (int i = 0; i < subforge::kNumPatchParams; ++i)
        values[(size_t) i] = apvts.getRawParameterValue (subforge::kPatchParamIds[i])->load();
    return values;
}

void SubForgeProcessor::applyPatch (const subforge::PatchValues& values)
{
    auto set = [this] (const char* id, float value)
    {
        auto* param = apvts.getParameter (id);
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (value));
        param->endChangeGesture();
    };

    for (int i = 0; i < subforge::kNumPatchParams; ++i)
        set (subforge::kPatchParamIds[i], values[(size_t) i]);
    set ("volume", subforge::normalizedVolumeDb (values));
}

void SubForgeProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, subforge::kNumPresets))
        return;

    currentPreset = index;
    patchName = subforge::kPresets[index].name;
    applyPatch (subforge::kPresets[index].values);
    sendChangeMessage();
}

void SubForgeProcessor::randomizePatch()
{
    applyPatch (subforge::randomPatch (rng));
    patchName = "Random " + juce::String (++randomCount);
    sendChangeMessage();
}

void SubForgeProcessor::mutatePatch()
{
    std::uniform_real_distribution<float> uniform (0.0f, 1.0f);
    std::normal_distribution<float> nudge (0.0f, 0.07f);

    auto values = currentPatch();
    for (int i = 0; i < subforge::kNumPatchParams; ++i)
    {
        auto* param = apvts.getParameter (subforge::kPatchParamIds[i]);
        auto& value = values[(size_t) i];

        if (i == subforge::pWave || i == subforge::pSubWave || i == subforge::pOctave)
        {
            // discrete choices only flip occasionally
            if (uniform (rng) < 0.1f)
                value = param->convertFrom0to1 (uniform (rng));
            value = std::round (value);
        }
        else
        {
            const float normalised = juce::jlimit (0.0f, 1.0f, param->convertTo0to1 (value) + nudge (rng));
            value = param->convertFrom0to1 (normalised);
        }
    }

    applyPatch (values);
    if (! patchName.endsWith ("*"))
        patchName << " *";
    sendChangeMessage();
}

void SubForgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("patchName", patchName, nullptr);
    state.setProperty ("presetIndex", currentPreset, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SubForgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            patchName = state.getProperty ("patchName", "Init").toString();
            currentPreset = juce::jlimit (0, subforge::kNumPresets - 1, (int) state.getProperty ("presetIndex", 0));
            apvts.replaceState (state);
            sendChangeMessage();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubForgeProcessor();
}
