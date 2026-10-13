#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/InputGain.h"
#include "../DSP/PreFilters.h"
#include "../DSP/TubePreamp.h"
#include "../DSP/ToneStack.h"
#include "../DSP/GraphicEQ.h"
#include "../DSP/MasterVolume.h"
#include "../DSP/PowerSection.h"

class AmpBassAudioProcessor final : public juce::AudioProcessor {
public:
    AmpBassAudioProcessor();
    ~AmpBassAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    // ── DSP chain (7 stages) ──
    InputGain     inputGain;
    PreFilters    preFilters;
    TubePreamp    tubePreamp;
    ToneStack     toneStack;
    GraphicEQ     graphicEQ;
    MasterVolume  masterVolume;
    PowerSection  powerSection;

    // ── Cached parameter pointers (set once in constructor) ──
    std::atomic<float>* paramInputPad   = nullptr;
    std::atomic<float>* paramInputHiLo  = nullptr;
    std::atomic<float>* paramBright     = nullptr;
    std::atomic<float>* paramUltraHi    = nullptr;
    std::atomic<float>* paramUltraLo    = nullptr;
    std::atomic<float>* paramGain       = nullptr;
    std::atomic<float>* paramTubeGain   = nullptr;
    std::atomic<float>* paramBass       = nullptr;
    std::atomic<float>* paramMidrange   = nullptr;
    std::atomic<float>* paramMidFreq    = nullptr;
    std::atomic<float>* paramTreble     = nullptr;
    std::atomic<float>* paramGeqOn      = nullptr;
    std::atomic<float>* paramGeq33      = nullptr;
    std::atomic<float>* paramGeq80      = nullptr;
    std::atomic<float>* paramGeq150     = nullptr;
    std::atomic<float>* paramGeq300     = nullptr;
    std::atomic<float>* paramGeq600     = nullptr;
    std::atomic<float>* paramGeq900     = nullptr;
    std::atomic<float>* paramGeq2k      = nullptr;
    std::atomic<float>* paramGeq5k      = nullptr;
    std::atomic<float>* paramGeq8k      = nullptr;
    std::atomic<float>* paramGeqLevel   = nullptr;
    std::atomic<float>* paramMaster     = nullptr;

    void updateDSPParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpBassAudioProcessor)
};
