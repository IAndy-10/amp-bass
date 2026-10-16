#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/InputGain.h"
#include "../DSP/PreFilters.h"
#include "../DSP/TubePreamp.h"
#include "../DSP/ToneStack.h"
#include "../DSP/GraphicEQ.h"
#include "../DSP/MasterVolume.h" 
#include "../DSP/PowerSection.h"
#include "../DSP/OutputSwitch.h"
#include "../DSP/Primitives/DryWetMixer.h"

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

    juce::AudioParameterBool* getBypassParameter() const override;

    /// Set on the audio thread when the input, preamp or EQ overloads.
    /// The editor reads and clears it to drive the PEAK/MUTE LED.
    std::atomic<bool> peakFlag { false };

private:
    // ── DSP chain (7 stages) ──
    InputGain     inputGain;
    PreFilters    preFilters;
    TubePreamp    tubePreamp;
    ToneStack     toneStack;
    GraphicEQ     graphicEQ;
    MasterVolume  masterVolume;
    PowerSection  powerSection;
    DryWetMixer   dryWetMixer;
    OutputSwitch  outputSwitch;

    // PEAK/MUTE LED thresholds (see processBlock)
    static constexpr float kPeakInput     = 0.98f; // raw input near full scale
    static constexpr float kPeakDrive     = 4.0f;  // preamp drive, tanh(4) = 0.999
    static constexpr float kPeakPreMaster = 1.0f;  // after tone stack + GEQ

    int preparedBlockSize = 512;   // max sub-block size passed to the DSP stages

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
    std::atomic<float>* paramDryWet     = nullptr;
    std::atomic<float>* paramBypass     = nullptr;
    std::atomic<float>* paramMute       = nullptr;

    void updateDSPParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpBassAudioProcessor)
};
