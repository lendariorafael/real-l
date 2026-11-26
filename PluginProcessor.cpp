#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const char* const kAudioPattern = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3";

    struct FactoryPreset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    // Starter presets. Parameter ids match createLayout(). "wave": 0 Saw, 1 Square, 2 Triangle, 3 Sine.
    const std::vector<FactoryPreset>& getFactoryPresets()
    {
        static const std::vector<FactoryPreset> presets {
            { "Warm Strings", { { "wave", 0.0f }, { "attack", 0.35f }, { "release", 1.2f }, { "cutoff", 3200.0f },
                                { "resonance", 0.15f }, { "unison", 14.0f }, { "vibRate", 4.5f }, { "vibDepth", 0.25f },
                                { "character", 0.1f }, { "reverbMix", 0.35f }, { "reverbSize", 0.7f },
                                { "delayTime", 420.0f }, { "delayFb", 0.25f }, { "delayMix", 0.1f } } },
            { "Electric Keys", { { "wave", 2.0f }, { "attack", 0.005f }, { "release", 0.6f }, { "cutoff", 6000.0f },
                                 { "unison", 4.0f }, { "vibRate", 5.5f }, { "vibDepth", 0.15f }, { "character", 0.25f },
                                 { "reverbMix", 0.25f }, { "reverbSize", 0.4f }, { "delayTime", 320.0f },
                                 { "delayFb", 0.35f }, { "delayMix", 0.25f } } },
            { "Dark Orchestra", { { "wave", 0.0f }, { "attack", 0.6f }, { "release", 2.0f }, { "cutoff", 1200.0f },
                                  { "resonance", 0.3f }, { "unison", 20.0f }, { "character", 0.2f },
                                  { "reverbMix", 0.5f }, { "reverbSize", 0.9f }, { "delayTime", 650.0f },
                                  { "delayFb", 0.4f }, { "delayMix", 0.15f } } },
            { "Glitch Form", { { "wave", 1.0f }, { "attack", 0.001f }, { "release", 0.08f }, { "cutoff", 2500.0f },
                               { "resonance", 0.45f }, { "gateRate", 8.0f }, { "gateAmt", 0.9f },
                               { "reverbMix", 0.2f }, { "delayTime", 125.0f }, { "delayFb", 0.6f }, { "delayMix", 0.4f } } },
            { "Euphoria Pad", { { "wave", 0.0f }, { "attack", 1.2f }, { "release", 3.0f }, { "cutoff", 9000.0f },
                                { "unison", 25.0f }, { "vibRate", 5.0f }, { "vibDepth", 0.3f },
                                { "reverbMix", 0.5f }, { "reverbSize", 0.85f }, { "delayTime", 500.0f },
                                { "delayFb", 0.3f }, { "delayMix", 0.2f } } },
            { "Gated Bass", { { "wave", 0.0f }, { "attack", 0.002f }, { "release", 0.15f }, { "cutoff", 700.0f },
                              { "resonance", 0.4f }, { "gateRate", 2.0f }, { "gateAmt", 0.6f }, { "character", 0.35f } } },
            { "Brass Section", { { "wave", 0.0f }, { "attack", 0.08f }, { "release", 0.25f }, { "cutoff", 2800.0f },
                                 { "resonance", 0.25f }, { "unison", 6.0f }, { "character", 0.45f },
                                 { "reverbMix", 0.2f }, { "reverbSize", 0.4f } } },
            { "Glass Bell", { { "wave", 3.0f }, { "attack", 0.001f }, { "release", 2.5f }, { "cutoff", 20000.0f },
                              { "reverbMix", 0.4f }, { "reverbSize", 0.8f }, { "delayTime", 480.0f },
                              { "delayFb", 0.45f }, { "delayMix", 0.3f } } },
        };
        return presets;
    }
}

//==============================================================================
RealLabProcessor::RealLabProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createLayout())
{
    for (int i = 0; i < 16; ++i)
    {
        synth.addVoice (new RealLab::SynthVoice());
        sampler.addVoice (new juce::SamplerVoice());
    }
    synth.addSound (new RealLab::SynthSound());
}

RealLabProcessor::~RealLabProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout RealLabProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params] (const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (lo, hi, 0.0f, skew), def));
    };

    // Master and envelope
    addFloat ("volume",    "Volume",      0.0f,   1.0f,    0.8f);
    addFloat ("attack",    "Attack",      0.001f, 3.0f,    0.01f, 0.25f);
    addFloat ("release",   "Release",     0.005f, 5.0f,    0.3f,  0.25f);

    // Source and filter
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "wave", 1 }, "Wave", juce::StringArray { "Saw", "Square", "Triangle", "Sine" }, 0));
    addFloat ("unison",    "Unison",      0.0f,   50.0f,   8.0f);
    addFloat ("cutoff",    "Filter",      20.0f,  20000.0f, 18000.0f, 0.25f);
    addFloat ("resonance", "Resonance",   0.0f,   1.0f,    0.1f);

    // Modulation
    addFloat ("vibRate",   "Vib Rate",    0.1f,   12.0f,   5.0f, 0.5f);
    addFloat ("vibDepth",  "Vibrato",     0.0f,   1.0f,    0.0f);
    addFloat ("gateRate",  "Gate Rate",   0.5f,   16.0f,   4.0f, 0.5f);
    addFloat ("gateAmt",   "Gated",       0.0f,   1.0f,    0.0f);

    // Character, reverb, delay
    addFloat ("character", "Character",   0.0f,   1.0f,    0.0f);
    addFloat ("reverbMix", "Reverb",      0.0f,   1.0f,    0.2f);
    addFloat ("reverbSize","Room",        0.0f,   1.0f,    0.5f);
    addFloat ("delayTime", "Delay Time",  1.0f,   1000.0f, 375.0f, 0.5f);
    addFloat ("delayFb",   "Feedback",    0.0f,   0.95f,   0.3f);
    addFloat ("delayMix",  "Delay",       0.0f,   1.0f,    0.2f);

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypassFX", 1 }, "Bypass FX", false));

    return { params.begin(), params.end() };
}

//==============================================================================
void RealLabProcessor::prepareToPlay (double newSampleRate, int)
{
    sr = newSampleRate;
    synth.setCurrentPlaybackSampleRate (sr);
    sampler.setCurrentPlaybackSampleRate (sr);

    reverb.setSampleRate (sr);
    reverb.reset();

    vibrato.prepare ((int) (0.05 * sr) + 16);
    delay.prepare ((int) (1.0 * sr) + 16);

    filterL.reset();
    filterR.reset();
    vibPhase = 0.0f;
    gatePhase = 0.0f;
    gateGain = 1.0f;
}

bool RealLabProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RealLabProcessor::updateVoices()
{
    const int wave        = (int) apvts.getRawParameterValue ("wave")->load();
    const float unison    = apvts.getRawParameterValue ("unison")->load();
    const float attack    = apvts.getRawParameterValue ("attack")->load();
    const float release   = apvts.getRawParameterValue ("release")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<RealLab::SynthVoice*> (synth.getVoice (i)))
            voice->setSettings (wave, unison, attack, release);
}

void RealLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    updateVoices();

    if (sampler.getNumSounds() > 0)
        sampler.renderNextBlock (buffer, midi, 0, numSamples);
    else
        synth.renderNextBlock (buffer, midi, 0, numSamples);

    if (buffer.getNumChannels() >= 2 && apvts.getRawParameterValue ("bypassFX")->load() < 0.5f)
        applyEffects (buffer);

    buffer.applyGain (apvts.getRawParameterValue ("volume")->load());
}

void RealLabProcessor::applyEffects (juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float cutoff     = get ("cutoff");
    const float q          = 0.5f + get ("resonance") * 8.0f;
    const float vibRate    = get ("vibRate");
    const float vibDepth   = get ("vibDepth");
    const float gateRate   = get ("gateRate");
    const float gateAmt    = get ("gateAmt");
    const float character  = get ("character");
    const float delayMs    = get ("delayTime");
    const float delayFb    = get ("delayFb");
    const float delayMix   = get ("delayMix");
    const float reverbMix  = get ("reverbMix");
    const float reverbSize = get ("reverbSize");

    filterL.setup (cutoff, q, sr);
    filterR.setup (cutoff, q, sr);

    const float vibBase     = 0.003f * (float) sr;
    const float vibRange    = 0.002f * (float) sr;
    const float vibInc      = vibRate / (float) sr;
    const float gateInc     = gateRate / (float) sr;
    const float delaySamples = delayMs * 0.001f * (float) sr;
    const float shapeG      = 1.0f + 6.0f * character;
    const float shapeNorm   = std::tanh (shapeG);

    for (int i = 0; i < n; ++i)
    {
        // Filter
        float l = filterL.process (L[i]);
        float r = filterR.process (R[i]);

        // Vibrato: modulated short delay, blended by depth
        vibPhase += vibInc;
        if (vibPhase >= 1.0f) vibPhase -= 1.0f;
        const float vibTime = vibBase + vibRange * std::sin (juce::MathConstants<float>::twoPi * vibPhase);
        const float lv = vibrato.read (0, vibTime);
        const float rv = vibrato.read (1, vibTime);
        vibrato.write (0, l);
        vibrato.write (1, r);
        vibrato.advance();
        l += (lv - l) * vibDepth;
        r += (rv - r) * vibDepth;

        // Gate: rhythmic volume chop
        gatePhase += gateInc;
        if (gatePhase >= 1.0f) gatePhase -= 1.0f;
        const float gateTarget = gatePhase < 0.5f ? 1.0f : 1.0f - gateAmt;
        gateGain += (gateTarget - gateGain) * 0.01f;
        l *= gateGain;
        r *= gateGain;

        // Character: soft saturation, blended
        l = (1.0f - character) * l + character * std::tanh (shapeG * l) / shapeNorm;
        r = (1.0f - character) * r + character * std::tanh (shapeG * r) / shapeNorm;

        // Delay with feedback
        const float dl = delay.read (0, delaySamples);
        const float dr = delay.read (1, delaySamples);
        delay.write (0, l + dl * delayFb);
        delay.write (1, r + dr * delayFb);
        delay.advance();
        l += dl * delayMix;
        r += dr * delayMix;

        L[i] = l;
        R[i] = r;
    }

    // Reverb (block based)
    juce::Reverb::Parameters p;
    p.roomSize   = reverbSize;
    p.damping    = 0.5f;
    p.wetLevel   = reverbMix;
    p.dryLevel   = 1.0f - 0.5f * reverbMix;
    p.width      = 1.0f;
    p.freezeMode = 0.0f;
    reverb.setParameters (p);
    reverb.processStereo (L, R, n);
}

//==============================================================================
void RealLabProcessor::setParam (const juce::String& id, float value)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (value));
}

void RealLabProcessor::applyFactoryPreset (int index)
{
    // Reset everything except the bypass switch, so presets never change it.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->paramID != "bypassFX")
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : getFactoryPresets()[(size_t) index].values)
        setParam (id, value);
}

void RealLabProcessor::loadPresetFile (const juce::File& file)
{
    if (auto xml = juce::parseXML (file))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void RealLabProcessor::applyPreset (const PresetEntry& entry)
{
    if (entry.factoryIndex >= 0)
        applyFactoryPreset (entry.factoryIndex);
    else
        loadPresetFile (entry.file);
}

void RealLabProcessor::savePresetTo (const juce::File& file)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        xml->writeTo (file);
}

juce::File RealLabProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("RealLab")
        .getChildFile ("Presets");
}

std::vector<RealLabProcessor::PresetEntry> RealLabProcessor::getPresetList() const
{
    std::vector<PresetEntry> list;

    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int) factory.size(); ++i)
    {
        PresetEntry e;
        e.name = factory[(size_t) i].name;
        e.factoryIndex = i;
        list.push_back (e);
    }

    auto folder = getUserPresetFolder();
    folder.createDirectory();
    for (const auto& f : folder.findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        PresetEntry e;
        e.name = f.getFileNameWithoutExtension();
        e.file = f;
        list.push_back (e);
    }

    return list;
}

//==============================================================================
bool RealLabProcessor::loadSampleFolder (const juce::File& target)
{
    std::vector<juce::File> files;

    if (target.isDirectory())
    {
        for (const auto& f : target.findChildFiles (juce::File::findFiles, false, kAudioPattern))
            files.push_back (f);
    }
    else if (target.existsAsFile())
    {
        files.push_back (target);
    }

    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileName() < b.getFileName();
    });

    if (files.empty())
    {
        sampleStatus = "No audio files found";
        return false;
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    sampler.allNotesOff (0, false);
    sampler.clearSounds();

    const int count = (int) files.size();
    const int lowest = 36, highest = 84;                     // C2 to C6
    const int span = juce::jmax (1, (highest - lowest + 1) / count);
    int loaded = 0;

    for (int i = 0; i < count; ++i)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (files[(size_t) i]));
        if (reader == nullptr)
            continue;

        juce::BigInteger keys;
        int root = 60;

        if (count == 1)
        {
            keys.setRange (0, 128, true);
        }
        else
        {
            const int from = juce::jmin (127, lowest + i * span);
            const int to   = juce::jmin (127, (i == count - 1) ? juce::jmax (from, highest) : from + span - 1);
            keys.setRange (from, to - from + 1, true);
            root = (from + to) / 2;
        }

        sampler.addSound (new juce::SamplerSound (files[(size_t) i].getFileNameWithoutExtension(),
                                                  *reader, keys, root, 0.002, 0.1, 30.0));
        ++loaded;
    }

    if (loaded == 0)
    {
        sampleStatus = "Could not read the audio files";
        return false;
    }

    sampleStatus = count == 1
        ? "Sample: " + files[0].getFileNameWithoutExtension()
        : juce::String (count) + " samples across C2 to C6";
    return true;
}

void RealLabProcessor::clearSamples()
{
    sampler.allNotesOff (0, false);
    sampler.clearSounds();
    sampleStatus = "Synth mode: no samples loaded";
}

//==============================================================================
void RealLabProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void RealLabProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RealLabProcessor::createEditor()
{
    return new RealLabEditor (*this);
}

//==============================================================================
// Plugin entry point used by the JUCE wrapper.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RealLabProcessor();
}
