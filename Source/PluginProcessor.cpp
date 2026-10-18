// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetFiles.h"

namespace
{
juce::NormalisableRange<float> skewedRange (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> range (lo, hi);
    range.setSkewForCentre (centre);
    return range;
}

// A note name as the Root shows it ("C#2", "Bb1", middle C is C3) back to its MIDI number, so the
// text a host shows reads back the same; a plain number is taken as the MIDI number.
int noteFromText (const juce::String& text)
{
    const auto t = text.trim().toUpperCase();
    const int letter = juce::String ("C D EF G A B").indexOfChar (t[0]);
    if (letter < 0 || t[0] == ' ')
        return t.getIntValue();

    int note = letter;
    int i = 1;
    if (t[i] == '#')
        ++note, ++i;
    else if (t[i] == 'B')
        --note, ++i;
    return note + (t.substring (i).getIntValue() + 2) * 12;
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
        // decided on the rounded value, so 9.97 shows as "10" like 10 does and the text reads back the same
        const float tenths = std::round (value * 10.0f) / 10.0f;
        const auto number = std::abs (tenths) < 10.0f ? juce::String (tenths, 1) : juce::String (juce::roundToInt (value));
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
// The sequencer parameters that edit the groove of the slot that plays.
constexpr const char* kGrooveParams[] = { "seqLength", "seqRate", "seqSwing", "seqGate" };

// Global (not per-project) preferences: ~/.config/SubForge/SubForge.settings on Linux.
juce::PropertiesFile::Options settingsOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "SubForge";
   #if JUCE_LINUX
    options.folderName = ".config/SubForge"; // JUCE puts Linux settings straight under ~ otherwise
   #else
    options.folderName = "SubForge";
   #endif
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}
} // namespace

SubForgeProcessor::SubForgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout()),
      settings (settingsOptions()),
      soundPresets (apvts, settings, [this] { sendChangeMessage(); }),
      patternPresets (apvts, patternBank,
                      [this] { return soundPresets.folder().getParentDirectory().getChildFile ("Patterns"); },
                      [this] { sendChangeMessage(); })
{
    slotNotes = settings.getBoolValue ("slotNotes", true);
    for (auto* id : kGrooveParams)
        apvts.addParameterListener (id, this);
    startTimerHz (20);
}

SubForgeProcessor::~SubForgeProcessor()
{
    stopTimer();
    for (auto* id : kGrooveParams)
        apvts.removeParameterListener (id, this);
}

void SubForgeProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // a value the timer shows, or one a loaded project sets, is not an edit
    if (restoringState.load() || (showingGroove && juce::MessageManager::existsAndIsCurrentThread()))
        return;

    auto& pattern = patternBank.activePattern();
    if (parameterID == "seqLength")
        pattern.setLength (juce::roundToInt (newValue));
    else if (parameterID == "seqRate")
        pattern.setRate (juce::roundToInt (newValue));
    else if (parameterID == "seqSwing")
        pattern.setSwing (newValue);
    else if (parameterID == "seqGate")
        pattern.setGate (newValue);
}

void SubForgeProcessor::timerCallback()
{
    // Hosts record these changes as automation while they record, change gesture or
    // not (CLAP and VST3 have no way for JUCE to say "don't record"). See the known
    // limitation in the README.
    const auto groove = patternBank.activePattern().groove();
    const juce::ScopedValueSetter<bool> showing (showingGroove, true);
    auto show = [this] (const char* id, float value)
    {
        if (std::abs (apvts.getRawParameterValue (id)->load() - value) > 1.0e-6f)
        {
            auto* param = apvts.getParameter (id);
            param->setValueNotifyingHost (param->convertTo0to1 (value));
        }
    };
    show ("seqLength", (float) groove.length);
    show ("seqRate", (float) groove.rate);
    show ("seqSwing", groove.swing);
    show ("seqGate", groove.gate);
}

juce::AudioProcessorValueTreeState::ParameterLayout SubForgeProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "wave", 1 }, "Wave",
                                                              StringArray { "Saw", "Square", "Pulse", "Sine", "Triangle" }, 0));
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
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "filterMode", 1 }, "Filter Mode",
                                                              StringArray { "LP 24", "LP 12", "BP 12", "Diode" }, 0));
    params.push_back (floatParam ("keyTrack", "Key Track", { 0.0f, 1.0f }, 0.0f));

    params.push_back (floatParam ("attack", "Attack", skewedRange (0.5f, 2000.0f, 50.0f), 1.0f, "ms"));
    params.push_back (floatParam ("decay", "Decay", skewedRange (5.0f, 5000.0f, 400.0f), 600.0f, "ms"));
    params.push_back (floatParam ("sustain", "Sustain", { 0.0f, 1.0f }, 0.7f));
    params.push_back (floatParam ("release", "Release", skewedRange (5.0f, 3000.0f, 200.0f), 60.0f, "ms"));

    params.push_back (floatParam ("glide", "Glide", skewedRange (0.0f, 1000.0f, 100.0f), 50.0f, "ms"));
    params.push_back (floatParam ("velocity", "Velocity Sens", { 0.0f, 1.0f }, 0.5f));
    params.push_back (floatParam ("accent", "Accent", { 0.0f, 1.0f }, 0.0f));
    params.push_back (floatParam ("accentSweep", "Accent Sweep", { 0.0f, 1.0f }, 0.0f));

    params.push_back (floatParam ("pitchEnv", "Pitch Env", { -24.0f, 24.0f }, 0.0f, "st"));
    params.push_back (floatParam ("pitchDecay", "Pitch Decay", skewedRange (5.0f, 1000.0f, 100.0f), 60.0f, "ms"));
    params.push_back (floatParam ("click", "Click", { 0.0f, 1.0f }, 0.0f));

    StringArray syncNames;
    for (auto* name : subforge::kLfoSyncNames)
        syncNames.add (name);
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "lfoShape", 1 }, "LFO Shape",
                                                              StringArray { "Sine", "Triangle", "Saw Down", "Square", "S&H" }, 0));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "lfoSync", 1 }, "LFO Sync", syncNames, 7));
    params.push_back (floatParam ("lfoRate", "LFO Rate", skewedRange (0.05f, 20.0f, 2.0f), 2.0f, "Hz"));
    params.push_back (floatParam ("lfoDepth", "LFO Depth", { 0.0f, 1.0f }, 0.0f));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { "lfoRetrig", 1 }, "LFO Retrigger", false));

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "distType", 1 }, "Distortion Type",
                                                              StringArray { "Off", "Overdrive", "Distortion", "Fuzz", "Tube Amp" }, 0));
    params.push_back (floatParam ("distDrive", "Distortion Gain", { 0.0f, 1.0f }, 0.5f));
    params.push_back (floatParam ("distTone", "Distortion Tone", { 0.0f, 1.0f }, 0.5f));
    params.push_back (floatParam ("distMix", "Distortion Mix", { 0.0f, 1.0f }, 1.0f));

    // Controller routing is part of the instance (and user presets), not of factory/random patches.
    const StringArray modDestinations { "Off", "Cutoff", "LFO", "Reso", "Dist", "Vibrato" };
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "wheelDest", 1 }, "Mod Wheel Dest",
                                                              modDestinations, subforge::kModLfoDepth));
    params.push_back (floatParam ("wheelAmount", "Mod Wheel Amount", { -1.0f, 1.0f }, 0.5f));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "aftertouchDest", 1 }, "Aftertouch Dest",
                                                              modDestinations, subforge::kModCutoff));
    params.push_back (floatParam ("aftertouchAmount", "Aftertouch Amount", { -1.0f, 1.0f }, 0.3f));

    StringArray rateNames;
    for (auto* name : subforge::kSeqRateNames)
        rateNames.add (name);
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { "seqOn", 1 }, "Sequencer On", false));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "seqRate", 1 }, "Sequencer Rate", rateNames, 1));
    params.push_back (std::make_unique<AudioParameterInt> (ParameterID { "seqLength", 1 }, "Sequencer Steps", 1,
                                                           subforge::kMaxSteps, subforge::kMaxSteps));
    params.push_back (floatParam ("seqGate", "Sequencer Gate", { 0.05f, 1.0f }, 0.5f));
    params.push_back (floatParam ("seqSwing", "Sequencer Swing", { 0.5f, 0.75f }, 0.5f));
    params.push_back (std::make_unique<AudioParameterInt> (
        ParameterID { "seqRoot", 1 }, "Sequencer Root", 24, 60, 36,
        AudioParameterIntAttributes()
            .withStringFromValueFunction ([] (int note, int) { return MidiMessage::getMidiNoteName (note, true, true, 3); })
            .withValueFromStringFunction ([] (const String& text) { return noteFromText (text); })));
    params.push_back (floatParam ("volume", "Volume", { -36.0f, 6.0f }, -6.0f, "dB"));

    // Added in 1.1.0, so last in the list and version 2. The number of slots must never
    // change: hosts store automation as 0..1 and would land on other slots.
    StringArray slotNames;
    for (int i = 0; i < subforge::kNumPatternSlots; ++i)
        slotNames.add (String::charToString ((juce_wchar) ('A' + i)));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "seqPattern", 2 }, "Sequencer Pattern", slotNames, 0));

    return { params.begin(), params.end() };
}

subforge::BassParams SubForgeProcessor::readParams() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    subforge::PatchValues patch {};
    for (int i = 0; i < subforge::kNumPatchParams; ++i)
        patch[(size_t) i] = get (subforge::kPatchParamIds[i]);

    auto p = subforge::toBassParams (patch, juce::Decibels::decibelsToGain (get ("volume")));
    p.wheelDest = (int) get ("wheelDest");
    p.wheelAmount = get ("wheelAmount");
    p.aftertouchDest = (int) get ("aftertouchDest");
    p.aftertouchAmount = get ("aftertouchAmount");
    return p;
}

void SubForgeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voiceMidi.ensureSize (8192);
    sequencer.reset();
    numTransposeKeys = 0;

    maxBlockSize = juce::jmax (1, samplesPerBlock);
    oversampler.initProcessing ((size_t) maxBlockSize);
    oversampler.reset();
    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()));

    synth.setParams (readParams());
    synth.prepare (sampleRate * kOversampling);
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
    else if (message.isControllerOfType (1))
        synth.setModWheel ((float) message.getControllerValue() / 127.0f);
    else if (message.isChannelPressure())
        synth.setAftertouch ((float) message.getChannelPressureValue() / 127.0f);
    else if (message.isAftertouch()) // polyphonic pressure: a mono voice just follows the latest
        synth.setAftertouch ((float) message.getAfterTouchValue() / 127.0f);
}

void SubForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    synth.setParams (readParams());

    double bpm = 120.0, ppq = 0.0;
    bool hasPosition = false;
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            if (auto hostBpm = position->getBpm())
                bpm = *hostBpm;
            if (auto hostPpq = position->getPpqPosition(); hostPpq && position->getIsPlaying())
            {
                ppq = *hostPpq;
                hasPosition = true;
            }
        }
    }
    synth.setTransport (bpm, ppq, hasPosition);

    const int numSamples = buffer.getNumSamples();
    buildVoiceMidi (midi, numSamples, bpm, ppq, hasPosition);

    buffer.clear();
    auto mono = juce::dsp::AudioBlock<float> (buffer).getSingleChannelBlock (0);
    auto midiIt = voiceMidi.cbegin();

    // The voice runs at kOversampling x the host rate; MIDI positions are scaled to match.
    for (int start = 0; start < numSamples; start += maxBlockSize)
    {
        const int length = juce::jmin (maxBlockSize, numSamples - start);
        auto chunk = mono.getSubBlock ((size_t) start, (size_t) length);
        float* up = oversampler.processSamplesUp (chunk).getChannelPointer (0);
        int position = 0;

        for (; midiIt != voiceMidi.cend(); ++midiIt)
        {
            const auto metadata = *midiIt;
            const int eventPos = juce::jlimit (0, numSamples, metadata.samplePosition) - start;
            if (eventPos >= length)
                break;
            if (eventPos > position)
            {
                synth.render (up + position * kOversampling, (eventPos - position) * kOversampling);
                position = eventPos;
            }
            handleMidi (metadata.getMessage());
        }

        if (position < length)
            synth.render (up + position * kOversampling, (length - position) * kOversampling);

        oversampler.processSamplesDown (chunk);
    }

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

// While the sequencer runs, played keys transpose the pattern (acid style) instead of
// sounding; otherwise MIDI passes straight through. Sequencer notes are merged in.
void SubForgeProcessor::buildVoiceMidi (const juce::MidiBuffer& midi, int numSamples, double bpm, double ppq, bool playing)
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const bool running = get ("seqOn") >= 0.5f && playing;
    sequencerRunning = running;

    // Automation, the GUI or a slot note asks for a slot; the latest request wins. The
    // parameter only counts when it changes, so a flat automation line leaves notes alone.
    if (const int slot = (int) get ("seqPattern"); slot != lastPatternParam.exchange (slot))
        patternBank.request (slot);

    voiceMidi.clear();
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (running && message.isNoteOnOrOff())
        {
            const int note = message.getNoteNumber();
            if (slotNotes.load() && subforge::isSlotNote (note))
            {
                if (message.isNoteOn())
                    patternBank.request (note - subforge::kFirstSlotNote);
                continue;
            }
            const auto end = transposeKeys.begin() + numTransposeKeys;
            numTransposeKeys = (int) (std::remove (transposeKeys.begin(), end, note) - transposeKeys.begin());
            if (message.isNoteOn() && numTransposeKeys < (int) transposeKeys.size())
                transposeKeys[(size_t) numTransposeKeys++] = note;
            continue;
        }
        voiceMidi.addEvent (message, metadata.samplePosition);
    }
    if (! running)
        numTransposeKeys = 0;

    subforge::SequencerSettings seqSettings;
    seqSettings.enabled = get ("seqOn") >= 0.5f;
    seqSettings.rootNote = numTransposeKeys > 0 ? transposeKeys[(size_t) numTransposeKeys - 1] : (int) get ("seqRoot");

    sequencer.process (playing, ppq, bpm, getSampleRate(), numSamples, seqSettings, patternBank,
                       [this] (int sample, bool on, int note, float velocity)
                       {
                           voiceMidi.addEvent (on ? juce::MidiMessage::noteOn (1, note, velocity)
                                                  : juce::MidiMessage::noteOff (1, note), sample);
                       });
}

juce::AudioProcessorEditor* SubForgeProcessor::createEditor()
{
    return new SubForgeEditor (*this);
}

void SubForgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", kStateVersion, nullptr);
    soundPresets.writeState (state);
    patternPresets.writeState (state);

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
            const int version = state.getProperty ("stateVersion", 1);
            juce::ignoreUnused (version); // migrations from older versions go here
            soundPresets.readState (state);
            restoringState = true;
            patternPresets.readState (state);
            // Only the parameters (child nodes) stay in the tree: getStateInformation adds the
            // rest again, so a reloaded state saves byte for byte the same.
            state.removeAllProperties (nullptr);
            apvts.replaceState (state);
            // A bool keeps the raw value a host sent (say 0.26) while the APVTS holds it as 0, so
            // replaceState sees nothing to change and the 0.26 would stay: snap bools to 0 or 1.
            for (auto* p : getParameters())
                if (auto* b = dynamic_cast<juce::AudioParameterBool*> (p))
                    b->setValueNotifyingHost (b->get() ? 1.0f : 0.0f);
            restoringState = false;

            // the project starts in the slot its Pattern parameter was saved on
            const int slot = (int) apvts.getRawParameterValue ("seqPattern")->load();
            lastPatternParam = slot;
            patternBank.select (slot);
            sendChangeMessage();
            // Tells the host every parameter may have changed, so it reads them all again
            // (CLAP: params rescan of the values, which the wrapper sends for a program change).
            updateHostDisplay (ChangeDetails().withProgramChanged (true));
        }
    }
}

void SubForgeProcessor::choosePatternSlot (int slot)
{
    subforge::files::setParameterValue (*apvts.getParameter ("seqPattern"), (float) slot);
    patternBank.request (slot);
    if (! sequencerRunning.load())
        patternBank.applyPending();
    sendChangeMessage();
}

void SubForgeProcessor::setSlotNotesEnabled (bool enabled)
{
    slotNotes = enabled;
    settings.setValue ("slotNotes", enabled);
    settings.saveIfNeeded();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubForgeProcessor();
}
