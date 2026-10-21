#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ParameterIDs.h"

/// JUCE 8 native WebView bridge using relay objects for bidirectional parameter sync.
///
/// Each parameter gets a typed relay (WebSliderRelay for floats, WebToggleButtonRelay
/// for bools; int selectors also use WebSliderRelay) plus a corresponding
/// ParameterAttachment that handles gesture begin/end automatically.
///
/// No Timer polling. No evaluateJavascript(). No juce:// URL intercept.
class NativeBridge
{
public:
    NativeBridge();

    /// Call after APVTS is constructed. Creates all attachments.
    void connectToAPVTS (juce::AudioProcessorValueTreeState& apvts);

    /// Returns WebBrowserComponent::Options with all relays registered.
    /// Pass to the WebBrowserComponent constructor.
    juce::WebBrowserComponent::Options buildOptions (const juce::WebBrowserComponent::Options& baseOptions) const;

private:
    // --- Float parameter relays (knobs/sliders) ---
    juce::WebSliderRelay gainRelay       { "gain" };
    juce::WebSliderRelay tubeGainRelay   { "tubeGain" };
    juce::WebSliderRelay bassRelay       { "bass" };
    juce::WebSliderRelay midrangeRelay   { "midrange" };
    juce::WebSliderRelay trebleRelay     { "treble" };
    juce::WebSliderRelay masterRelay     { "master" };

    // GEQ sliders
    juce::WebSliderRelay geq33Relay      { "geq33" };
    juce::WebSliderRelay geq80Relay      { "geq80" };
    juce::WebSliderRelay geq150Relay     { "geq150" };
    juce::WebSliderRelay geq300Relay     { "geq300" };
    juce::WebSliderRelay geq600Relay     { "geq600" };
    juce::WebSliderRelay geq900Relay     { "geq900" };
    juce::WebSliderRelay geq2kRelay      { "geq2k" };
    juce::WebSliderRelay geq5kRelay      { "geq5k" };
    juce::WebSliderRelay geq8kRelay      { "geq8k" };
    juce::WebSliderRelay geqLevelRelay   { "geqLevel" };
    juce::WebSliderRelay dryWetRelay     { "dryWet" };

    // --- Bool parameter relays (toggle buttons) ---
    juce::WebToggleButtonRelay inputPadRelay  { "inputPad" };
    juce::WebToggleButtonRelay brightRelay    { "bright" };
    juce::WebToggleButtonRelay ultraHiRelay   { "ultraHi" };
    juce::WebToggleButtonRelay ultraLoRelay   { "ultraLo" };
    juce::WebToggleButtonRelay geqOnRelay     { "geqOn" };
    juce::WebToggleButtonRelay bypassRelay    { "bypass" };
    juce::WebToggleButtonRelay muteRelay      { "mute" };
    juce::WebToggleButtonRelay cabinetRelay   { "cabinet" };
    juce::WebToggleButtonRelay hornRelay      { "horn" };
    juce::WebToggleButtonRelay gateRelay      { "gate" };

    // --- Int parameter relays (combo box / selector) ---
    juce::WebSliderRelay midFreqRelay        { "midFreq" };
    juce::WebSliderRelay presetSelectRelay   { "presetSelect" };
    juce::WebSliderRelay tightRelay          { "tight" };
    juce::WebSliderRelay tightFreqRelay      { "tightFreq" };
    juce::WebSliderRelay powerSoakRelay      { "powerSoak" };
    juce::WebSliderRelay smoothRelay         { "smooth" };
    juce::WebSliderRelay smoothFreqRelay     { "smoothFreq" };
    juce::WebSliderRelay inputGainRelay      { "inputGain" };
    juce::WebSliderRelay gateThresholdRelay  { "gateThreshold" };
    juce::WebSliderRelay gateRangeRelay      { "gateRange" };

    // --- Parameter attachments (own gesture begin/end) ---
    // Stored as unique_ptrs because they're constructed after APVTS is ready.
    std::unique_ptr<juce::WebSliderParameterAttachment> gainAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> tubeGainAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> bassAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> midrangeAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> trebleAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> masterAttach;

    std::unique_ptr<juce::WebSliderParameterAttachment> geq33Attach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq80Attach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq150Attach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq300Attach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq600Attach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq900Attach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq2kAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq5kAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geq8kAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> geqLevelAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryWetAttach;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> inputPadAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> brightAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> ultraHiAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> ultraLoAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> geqOnAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> muteAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> cabinetAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> hornAttach;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> gateAttach;

    std::unique_ptr<juce::WebSliderParameterAttachment> midFreqAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> presetSelectAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> tightAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> tightFreqAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> powerSoakAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> smoothAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> smoothFreqAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> inputGainAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> gateThresholdAttach;
    std::unique_ptr<juce::WebSliderParameterAttachment> gateRangeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NativeBridge)
};
