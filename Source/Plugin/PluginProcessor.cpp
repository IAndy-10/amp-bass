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
    paramInputHiLo = apvts.getRawParameterValue (ParameterIDs::inputHiLo);
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
    inputGain.prepare (sampleRate, samplesPerBlock);
    preFilters.prepare (sampleRate, samplesPerBlock);
    tubePreamp.prepare (sampleRate, samplesPerBlock);
    toneStack.prepare (sampleRate, samplesPerBlock);
    graphicEQ.prepare (sampleRate, samplesPerBlock);
    masterVolume.prepare (sampleRate, samplesPerBlock);
    powerSection.prepare (sampleRate, samplesPerBlock);
    dryWetMixer.prepare (2, samplesPerBlock);

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
}

void AmpBassAudioProcessor::updateDSPParameters() {
    // Stage 1: InputGain
    inputGain.setParams (paramInputPad->load() >= 0.5f,
                         paramInputHiLo->load() >= 0.5f);

    // Stage 2: PreFilters
    preFilters.setParams (paramBright->load() >= 0.5f,
                          paramUltraHi->load() >= 0.5f,
                          paramUltraLo->load() >= 0.5f);

    // Stage 3: TubePreamp
    tubePreamp.setParams (paramGain->load(), paramTubeGain->load());

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

    // Global dry/wet
    dryWetMixer.setMix (paramDryWet->load());
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

    // ── Save dry signal for dry/wet mix ──
    dryWetMixer.saveDry (buffer);

    // ── Signal chain (7 stages) ──
    inputGain.process (buffer);      // Stage 1
    preFilters.process (buffer);     // Stage 2
    tubePreamp.process (buffer);     // Stage 3
    toneStack.process (buffer);      // Stage 4
    graphicEQ.process (buffer);      // Stage 5
    masterVolume.process (buffer);   // Stage 6
    powerSection.process (buffer);   // Stage 7

    // ── Blend dry/wet (equal-power crossfade) ──
    dryWetMixer.mixToOutput (buffer);
}

bool AmpBassAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AmpBassAudioProcessor::createEditor() {
    return new AmpBassAudioProcessorEditor(*this);
}

void AmpBassAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AmpBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AmpBassAudioProcessor();
}
