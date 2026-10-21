#include "PluginProcessor.h"
#include "../Parameters/ParameterLayout.h"
#include "../Parameters/ParameterIDs.h"
#include "PluginEditor.h"

using namespace ParameterIDs;

AmpBassAudioProcessor::AmpBassAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input",  juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache raw parameter pointers (safe — APVTS lifetime matches processor)
    paramInputPad  = apvts.getRawParameterValue (ParameterIDs::inputPad);
    paramBright    = apvts.getRawParameterValue (ParameterIDs::bright);
    paramUltraHi   = apvts.getRawParameterValue (ParameterIDs::ultraHi);
    paramUltraLo   = apvts.getRawParameterValue (ParameterIDs::ultraLo);
    paramGain      = apvts.getRawParameterValue (ParameterIDs::gain);
    paramTubeGain  = apvts.getRawParameterValue (ParameterIDs::tubeGain);
    paramBass      = apvts.getRawParameterValue (ParameterIDs::bass);
    paramMidrange  = apvts.getRawParameterValue (ParameterIDs::midrange);
    paramMidFreq   = apvts.getRawParameterValue (ParameterIDs::midFreq);
    paramTreble    = apvts.getRawParameterValue (ParameterIDs::treble);
    paramGeqOn     = apvts.getRawParameterValue (ParameterIDs::geqOn);
    paramGeq33     = apvts.getRawParameterValue (ParameterIDs::geq33);
    paramGeq80     = apvts.getRawParameterValue (ParameterIDs::geq80);
    paramGeq150    = apvts.getRawParameterValue (ParameterIDs::geq150);
    paramGeq300    = apvts.getRawParameterValue (ParameterIDs::geq300);
    paramGeq600    = apvts.getRawParameterValue (ParameterIDs::geq600);
    paramGeq900    = apvts.getRawParameterValue (ParameterIDs::geq900);
    paramGeq2k     = apvts.getRawParameterValue (ParameterIDs::geq2k);
    paramGeq5k     = apvts.getRawParameterValue (ParameterIDs::geq5k);
    paramGeq8k     = apvts.getRawParameterValue (ParameterIDs::geq8k);
    paramGeqLevel  = apvts.getRawParameterValue (ParameterIDs::geqLevel);
    paramMaster    = apvts.getRawParameterValue (ParameterIDs::master);
    paramDryWet    = apvts.getRawParameterValue (ParameterIDs::dryWet);
    paramBypass    = apvts.getRawParameterValue (ParameterIDs::bypass);
    paramMute      = apvts.getRawParameterValue (ParameterIDs::mute);
    paramCabinet   = apvts.getRawParameterValue (ParameterIDs::cabinet);
    paramHorn      = apvts.getRawParameterValue (ParameterIDs::horn);
    paramTight     = apvts.getRawParameterValue (ParameterIDs::tight);
    paramTightFreq = apvts.getRawParameterValue (ParameterIDs::tightFreq);
    paramPowerSoak = apvts.getRawParameterValue (ParameterIDs::powerSoak);
    paramSmooth    = apvts.getRawParameterValue (ParameterIDs::smooth);
    paramSmoothFreq = apvts.getRawParameterValue (ParameterIDs::smoothFreq);
    paramInputGain = apvts.getRawParameterValue (ParameterIDs::inputGain);
    paramGate      = apvts.getRawParameterValue (ParameterIDs::gate);
    paramGateThreshold = apvts.getRawParameterValue (ParameterIDs::gateThreshold);
    paramGateRange = apvts.getRawParameterValue (ParameterIDs::gateRange);
}

AmpBassAudioProcessor::~AmpBassAudioProcessor() {}

const juce::String AmpBassAudioProcessor::getName() const { return JucePlugin_Name; }
bool AmpBassAudioProcessor::acceptsMidi()  const { return false; }
bool AmpBassAudioProcessor::producesMidi() const { return false; }
bool AmpBassAudioProcessor::isMidiEffect() const { return false; }
double AmpBassAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AmpBassAudioProcessor::getNumPrograms() { return 1; }
int AmpBassAudioProcessor::getCurrentProgram() { return 0; }
void AmpBassAudioProcessor::setCurrentProgram(int) {}
const juce::String AmpBassAudioProcessor::getProgramName(int) { return {}; }
void AmpBassAudioProcessor::changeProgramName(int, const juce::String&) {}

bool AmpBassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}

void AmpBassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    preparedBlockSize = juce::jmax (1, samplesPerBlock);

    // Set the targets first: PowerSection starts at its target in prepare(), so Tube Gain doesn't glide
    // (200 ms) from its default when playback starts.
    updateDSPParameters();

    inputGain.prepare (sampleRate, samplesPerBlock);
    tightPre.prepare (sampleRate, samplesPerBlock);
    smoothPre.prepare (sampleRate, samplesPerBlock);
    preFilters.prepare (sampleRate, samplesPerBlock);
    tubePreamp.prepare (sampleRate, samplesPerBlock);
    toneStack.prepare (sampleRate, samplesPerBlock);
    graphicEQ.prepare (sampleRate, samplesPerBlock);
    masterVolume.prepare (sampleRate, samplesPerBlock);
    powerSection.prepare (sampleRate, samplesPerBlock);
    tightPost.prepare (sampleRate, samplesPerBlock);
    smoothPost.prepare (sampleRate, samplesPerBlock);
    cabinet.prepare (sampleRate, samplesPerBlock);
    powerSoak.prepare (sampleRate, samplesPerBlock);
    noiseGate.prepare (sampleRate, samplesPerBlock);
    dryWetMixer.prepare (sampleRate, 2, samplesPerBlock);
    outputSwitch.prepare (sampleRate, samplesPerBlock);

    updateDSPParameters();
}

void AmpBassAudioProcessor::releaseResources() {
    inputGain.reset();
    preFilters.reset();
    tubePreamp.reset();
    toneStack.reset();
    graphicEQ.reset();
    masterVolume.reset();
    powerSection.reset();
    tightPre.reset();
    tightPost.reset();
    smoothPre.reset();
    smoothPost.reset();
    cabinet.reset();
    powerSoak.reset();
    noiseGate.reset();
    outputSwitch.reset();
}

juce::AudioParameterBool* AmpBassAudioProcessor::getBypassParameter() const
{
    // Lets hosts that support it map their own bypass button to this parameter.
    return dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParameterIDs::bypass));
}

void AmpBassAudioProcessor::updateDSPParameters() {
    // Stage 1: InputGain
    inputGain.setParams (paramInputPad->load() >= 0.5f, paramInputGain->load());

    // Stage 2: PreFilters
    preFilters.setParams (paramBright->load() >= 0.5f,
                          paramUltraHi->load() >= 0.5f,
                          paramUltraLo->load() >= 0.5f);

    // Stage 3: TubePreamp
    tubePreamp.setParams (paramGain->load());

    // Stage 4: ToneStack
    toneStack.setParams (paramBass->load(),
                         paramMidrange->load(),
                         static_cast<int> (paramMidFreq->load()),
                         paramTreble->load());

    // Stage 5: GraphicEQ
    const float bandDb[GraphicEQ::kNumBands] = {
        paramGeq33->load(),  paramGeq80->load(),  paramGeq150->load(),
        paramGeq300->load(), paramGeq600->load(), paramGeq900->load(),
        paramGeq2k->load(),  paramGeq5k->load(),  paramGeq8k->load()
    };
    graphicEQ.setParams (paramGeqOn->load() >= 0.5f, bandDb, paramGeqLevel->load());

    // Stage 6: MasterVolume
    masterVolume.setParams (paramMaster->load());

    // Stage 7: PowerSection (Tube Gain acts here, as in the reference)
    powerSection.setParams (paramTubeGain->load());

    // Stage 8: Cabinet
    cabinet.setParams (paramCabinet->load() >= 0.5f, paramHorn->load() >= 0.5f);

    // FX Rack: Tight (0 = Off, 1 = Pre, 2 = Post) and Power Soak
    const int tightMode = static_cast<int> (paramTight->load() + 0.5f);
    tightPre.setParams (tightMode == 1, paramTightFreq->load());
    tightPost.setParams (tightMode == 2, paramTightFreq->load());
    const int smoothMode = static_cast<int> (paramSmooth->load() + 0.5f);   // same choices; frequency in kHz
    smoothPre.setParams (smoothMode == 1, 1000.0f * paramSmoothFreq->load());
    smoothPost.setParams (smoothMode == 2, 1000.0f * paramSmoothFreq->load());
    powerSoak.setParams (paramPowerSoak->load());
    noiseGate.setParams (paramGate->load() >= 0.5f, paramGateThreshold->load(), paramGateRange->load(),
                         paramInputGain->load());

    // Global dry/wet
    dryWetMixer.setMix (paramDryWet->load());

    // Output switches
    outputSwitch.setParams (paramBypass->load() >= 0.5f, paramMute->load() >= 0.5f);
}

void AmpBassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Clear any output channels that have no corresponding input
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = getTotalNumInputChannels(); i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // ── Update parameter targets from APVTS ──
    updateDSPParameters();

    // ── Process in sub-blocks no larger than prepareToPlay's block size ──
    // Some hosts deliver blocks larger than announced; this keeps the
    // dry/wet buffer from reallocating on the audio thread.
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int start = 0; start < numSamples; start += preparedBlockSize)
    {
        const int n = juce::jmin (preparedBlockSize, numSamples - start);
        juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), numChannels, start, n);

        // ── Save dry signal for dry/wet mix ──
        dryWetMixer.saveDry (sub);

        // ── Signal chain (8 stages) ──
        inputGain.process (sub);      // Stage 1
        tightPre.process (sub);       // FX Rack Tight, Pre
        smoothPre.process (sub);      // FX Rack Smooth, Pre
        preFilters.process (sub);     // Stage 2
        tubePreamp.process (sub);     // Stage 3
        toneStack.process (sub);      // Stage 4
        graphicEQ.process (sub);      // Stage 5

        // ── PEAK: input clipping, preamp driven into hard saturation,
        //    or tone/EQ boosts pushing the signal past full scale ──
        //    All channels are checked. Nothing is reported while fully bypassed.
        //    takePeakDrive() also resets the preamp's peak, so call it every sub-block.
        const float peakDrive = tubePreamp.takePeakDrive();
        const bool overload = dryWetMixer.getDry().getMagnitude (0, n) >= kPeakInput
                           || peakDrive >= kPeakDrive
                           || sub.getMagnitude (0, n) >= kPeakPreMaster;
        if (overload && ! outputSwitch.isFullyBypassed())
            peakFlag.store (true, std::memory_order_relaxed);

        masterVolume.process (sub);   // Stage 6
        powerSection.process (sub);   // Stage 7
        tightPost.process (sub);      // FX Rack Tight, Post
        smoothPost.process (sub);     // FX Rack Smooth, Post
        noiseGate.process (sub, dryWetMixer.getDry());   // FX Rack Noise Gate: hears the input, turns the amp down
        cabinet.process (sub);        // Stage 8
        powerSoak.process (sub);      // FX Rack Power Soak

        // ── Blend dry/wet (equal-power crossfade) ──
        dryWetMixer.mixToOutput (sub);

        // ── Bypass (crossfade to dry input) and mute ──
        outputSwitch.process (sub, dryWetMixer.getDry());
    }
}

bool AmpBassAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AmpBassAudioProcessor::createEditor() {
    return new AmpBassAudioProcessorEditor(*this);
}

// Saved-state format. Bump it when a parameter's range or meaning changes, and convert older
// states in setStateInformation so existing sessions keep sounding the same.
// 1 = no version attribute (before the reference-matched ranges), 2 = reference-matched ranges,
// 3 = cabinet added, 4 = Tight and Power Soak added, 5 = Smooth added (and Input Gain, neutral by default),
// 6 = Noise Gate added, 7 = Input Select (HI/LO) removed.
static constexpr int kStateVersion = 7;
static const juce::Identifier kStateVersionId { "stateVersion" };

void AmpBassAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty(kStateVersionId, kStateVersion, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AmpBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (! xmlState || ! xmlState->hasTagName(apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml(*xmlState);
    const int version = state.getProperty(kStateVersionId, 1);
    // Version 1 states came from unreleased builds only; they load as-is (the user re-saves them).
    // Sessions from before a stage existed keep sounding the same: the stage loads off / neutral.
    auto setValue = [&state](const char* id, float value) {
        auto p = state.getChildWithProperty("id", id);
        if (! p.isValid())
        {
            p = juce::ValueTree("PARAM");
            p.setProperty("id", id, nullptr);
            state.appendChild(p, nullptr);
        }
        p.setProperty("value", value, nullptr);
    };
    if (version < 3)
        setValue(ParameterIDs::cabinet, 0.0f);
    if (version < 4)
    {
        setValue(ParameterIDs::tight, 0.0f);       // Off
        setValue(ParameterIDs::powerSoak, 0.0f);   // 0 dB
    }
    if (version < 5)
        setValue(ParameterIDs::smooth, 0.0f);      // Off
    if (version < 6)
        setValue(ParameterIDs::gate, 0.0f);        // Off
    // The removed Input Select: a session saved on LO (-6 dB) gets the same level from Input Gain.
    if (auto hiLo = state.getChildWithProperty("id", "inputHiLo"); hiLo.isValid())
    {
        if (static_cast<float>(hiLo.getProperty("value", 1.0f)) < 0.5f)
        {
            const float inputDb = state.getChildWithProperty("id", ParameterIDs::inputGain).getProperty("value", 0.0f);
            setValue(ParameterIDs::inputGain, juce::jmax(-20.0f, inputDb - 6.0f));
        }
        state.removeChild(hiLo, nullptr);
    }
    apvts.replaceState(state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AmpBassAudioProcessor();
}
