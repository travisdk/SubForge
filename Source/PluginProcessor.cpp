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
// JUCE's Linux File::moveToTrash() skips the .trashinfo file, so file managers
// can't show or restore the file. This follows the freedesktop.org trash spec.
bool moveToTrash (const juce::File& file)
{
   #if JUCE_LINUX
    const auto dataHome = juce::SystemStats::getEnvironmentVariable ("XDG_DATA_HOME", {});
    const auto trash = (dataHome.isNotEmpty() ? juce::File (dataHome)
                                              : juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                                    .getChildFile (".local/share"))
                           .getChildFile ("Trash");
    const auto files = trash.getChildFile ("files"), info = trash.getChildFile ("info");

    if (files.createDirectory() && info.createDirectory())
    {
        const auto target = files.getNonexistentChildFile (file.getFileNameWithoutExtension(), file.getFileExtension(), false);
        const auto infoFile = info.getChildFile (target.getFileName() + ".trashinfo");
        auto segments = juce::StringArray::fromTokens (file.getFullPathName(), "/", {});
        for (auto& segment : segments)
            segment = juce::URL::addEscapeChars (segment, false); // escape the parts, keep the slashes
        const auto infoText = "[Trash Info]\nPath=" + segments.joinIntoString ("/")
                              + "\nDeletionDate=" + juce::Time::getCurrentTime().formatted ("%Y-%m-%dT%H:%M:%S") + "\n";

        if (infoFile.replaceWithText (infoText))
        {
            if (file.moveFileTo (target))
                return true;
            infoFile.deleteFile();
        }
    }
   #endif
    return file.moveToTrash();
}
} // namespace

SubForgeProcessor::SubForgeProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout()),
      settings (settingsOptions())
{
    rescanUserPresets();
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
            .withValueFromStringFunction ([] (const String& text) { return text.getIntValue(); })));
    params.push_back (floatParam ("volume", "Volume", { -36.0f, 6.0f }, -6.0f, "dB"));

    return { params.begin(), params.end() };
}

subforge::BassParams SubForgeProcessor::readParams() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    auto p = subforge::toBassParams (currentPatch(), juce::Decibels::decibelsToGain (get ("volume")));
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

// While the sequencer runs, played keys transpose the pattern (303 style) instead of
// sounding; otherwise MIDI passes straight through. Sequencer notes are merged in.
void SubForgeProcessor::buildVoiceMidi (const juce::MidiBuffer& midi, int numSamples, double bpm, double ppq, bool playing)
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const bool running = get ("seqOn") >= 0.5f && playing;

    voiceMidi.clear();
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (running && message.isNoteOnOrOff())
        {
            const int note = message.getNoteNumber();
            auto* end = transposeKeys.begin() + numTransposeKeys;
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
    seqSettings.stepBeats = subforge::kSeqRateBeats[juce::jlimit (0, subforge::kNumSeqRates - 1, (int) get ("seqRate"))];
    seqSettings.length = (int) get ("seqLength");
    seqSettings.gate = get ("seqGate");
    seqSettings.swing = get ("seqSwing");
    seqSettings.rootNote = numTransposeKeys > 0 ? transposeKeys[(size_t) numTransposeKeys - 1] : (int) get ("seqRoot");

    sequencer.process (playing, ppq, bpm, getSampleRate(), numSamples, seqSettings, pattern,
                       [this] (int sample, bool on, int note, float velocity)
                       {
                           voiceMidi.addEvent (on ? juce::MidiMessage::noteOn (1, note, velocity)
                                                  : juce::MidiMessage::noteOff (1, note), sample);
                       });
}

void SubForgeProcessor::randomizePattern()
{
    pattern.randomize (rng);
    sendChangeMessage();
}

void SubForgeProcessor::clearPattern()
{
    pattern.clear();
    sendChangeMessage();
}

juce::AudioProcessorEditor* SubForgeProcessor::createEditor()
{
    return new SubForgeEditor (*this);
}

const juce::String SubForgeProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, subforge::kNumPresets) ? subforge::kPresets[index].name : "";
}

juce::String SubForgeProcessor::getEntryName (int index) const
{
    if (isUserEntry (index))
        return userPresets[index - subforge::kNumPresets].getFileNameWithoutExtension();
    return juce::isPositiveAndBelow (index, subforge::kNumPresets) ? subforge::kPresets[index].name : "";
}

juce::File SubForgeProcessor::defaultPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("SubForge/Presets");
}

juce::File SubForgeProcessor::getPresetFolder() const
{
    const auto custom = settings.getValue ("presetFolder");
    return custom.isNotEmpty() ? juce::File (custom) : defaultPresetFolder();
}

void SubForgeProcessor::setPresetFolder (const juce::File& folder)
{
    settings.setValue ("presetFolder", folder == juce::File() ? juce::String() : folder.getFullPathName());
    settings.saveIfNeeded();
    if (isUserEntry (currentEntry))
        currentEntry = 0;
    rescanUserPresets();
}

juce::File SubForgeProcessor::presetFileFor (const juce::String& name) const
{
    return getPresetFolder().getChildFile (juce::File::createLegalFileName (name.trim()) + kPresetExtension);
}

void SubForgeProcessor::rescanUserPresets()
{
    const auto currentFile = isUserEntry (currentEntry) ? userPresets[currentEntry - subforge::kNumPresets] : juce::File();

    userPresets = getPresetFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + kPresetExtension);
    std::sort (userPresets.begin(), userPresets.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0; });

    if (currentFile != juce::File())
    {
        const int index = userPresets.indexOf (currentFile);
        currentEntry = index >= 0 ? subforge::kNumPresets + index : 0;
    }
    sendChangeMessage();
}

void SubForgeProcessor::setParameterValue (juce::RangedAudioParameter& param, float value)
{
    param.beginChangeGesture();
    param.setValueNotifyingHost (param.convertTo0to1 (value));
    param.endChangeGesture();
}

// A user preset stores every parameter as a plain value, including volume:
// unlike factory presets it is restored exactly as it was saved.
bool SubForgeProcessor::saveUserPreset (const juce::String& name)
{
    const auto file = presetFileFor (name);
    if (! file.getParentDirectory().createDirectory())
        return false;

    juce::XmlElement xml ("SubForgePreset");
    xml.setAttribute ("version", 1);
    xml.setAttribute ("name", name.trim());
    xml.createNewChildElement ("Pattern")->setAttribute ("steps", juce::String (pattern.toString()));
    for (auto* p : getParameters())
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto* element = xml.createNewChildElement ("Param");
            element->setAttribute ("id", param->paramID);
            element->setAttribute ("value", param->convertFrom0to1 (param->getValue()));
        }

    if (! xml.writeTo (file))
        return false;

    patchName = file.getFileNameWithoutExtension();
    currentEntry = -1;
    rescanUserPresets();
    currentEntry = subforge::kNumPresets + userPresets.indexOf (file);
    sendChangeMessage();
    return true;
}

bool SubForgeProcessor::loadUserPresetFile (const juce::File& file)
{
    const auto xml = juce::parseXML (file);
    if (xml == nullptr || ! xml->hasTagName ("SubForgePreset"))
        return false;

    // parameters missing from older files fall back to their defaults
    for (auto* p : getParameters())
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            float value = param->convertFrom0to1 (param->getDefaultValue());
            if (auto* element = xml->getChildByAttribute ("id", param->paramID))
                value = (float) element->getDoubleAttribute ("value", value);
            setParameterValue (*param, value);
        }

    if (auto* patternElement = xml->getChildByName ("Pattern")) // older presets keep the current pattern
        pattern.fromString (patternElement->getStringAttribute ("steps").toStdString());
    return true;
}

void SubForgeProcessor::deleteUserPreset (int index)
{
    if (! isUserEntry (index))
        return;

    moveToTrash (userPresets[index - subforge::kNumPresets]);
    if (currentEntry == index)
        currentEntry = 0;
    rescanUserPresets();
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
    for (int i = 0; i < subforge::kNumPatchParams; ++i)
        setParameterValue (*apvts.getParameter (subforge::kPatchParamIds[i]), values[(size_t) i]);
    setParameterValue (*apvts.getParameter ("volume"), subforge::normalizedVolumeDb (values));
}

void SubForgeProcessor::loadEntry (int index)
{
    if (isUserEntry (index))
    {
        if (! loadUserPresetFile (userPresets[index - subforge::kNumPresets]))
            return;
    }
    else if (juce::isPositiveAndBelow (index, subforge::kNumPresets))
    {
        applyPatch (subforge::kPresets[index].values());
    }
    else
    {
        return;
    }

    currentEntry = index;
    patchName = getEntryName (index);
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

        if (dynamic_cast<juce::AudioParameterChoice*> (param) != nullptr
            || dynamic_cast<juce::AudioParameterInt*> (param) != nullptr
            || dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
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
    state.setProperty ("presetIndex", isUserEntry (currentEntry) ? 0 : currentEntry, nullptr);
    state.setProperty ("pattern", juce::String (pattern.toString()), nullptr);

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
            if (state.hasProperty ("pattern"))
                pattern.fromString (state.getProperty ("pattern").toString().toStdString());
            currentEntry = juce::jlimit (0, subforge::kNumPresets - 1, (int) state.getProperty ("presetIndex", 0));
            for (int i = subforge::kNumPresets; i < getNumEntries(); ++i)
                if (getEntryName (i) == patchName)
                    currentEntry = i; // a user preset of that name is the better match
            apvts.replaceState (state);
            sendChangeMessage();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubForgeProcessor();
}
