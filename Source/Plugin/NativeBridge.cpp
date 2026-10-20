#include "NativeBridge.h"

NativeBridge::NativeBridge() {}

void NativeBridge::connectToAPVTS (juce::AudioProcessorValueTreeState& apvts)
{
    auto& params = apvts;

    // --- Float attachments (WebSliderParameterAttachment handles beginGesture/endGesture) ---
    gainAttach     = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::gain),     gainRelay);
    tubeGainAttach = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::tubeGain), tubeGainRelay);
    bassAttach     = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::bass),     bassRelay);
    midrangeAttach = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::midrange), midrangeRelay);
    trebleAttach   = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::treble),   trebleRelay);
    masterAttach   = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::master),   masterRelay);

    geq33Attach    = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq33),    geq33Relay);
    geq80Attach    = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq80),    geq80Relay);
    geq150Attach   = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq150),   geq150Relay);
    geq300Attach   = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq300),   geq300Relay);
    geq600Attach   = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq600),   geq600Relay);
    geq900Attach   = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq900),   geq900Relay);
    geq2kAttach    = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq2k),    geq2kRelay);
    geq5kAttach    = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq5k),    geq5kRelay);
    geq8kAttach    = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geq8k),    geq8kRelay);
    geqLevelAttach = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::geqLevel), geqLevelRelay);
    dryWetAttach   = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::dryWet),   dryWetRelay);

    // --- Bool attachments ---
    inputPadAttach  = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::inputPad),  inputPadRelay);
    inputHiLoAttach = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::inputHiLo), inputHiLoRelay);
    brightAttach    = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::bright),    brightRelay);
    ultraHiAttach   = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::ultraHi),   ultraHiRelay);
    ultraLoAttach   = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::ultraLo),   ultraLoRelay);
    geqOnAttach     = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::geqOn),     geqOnRelay);
    bypassAttach    = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::bypass),    bypassRelay);
    muteAttach      = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::mute),      muteRelay);
    cabinetAttach   = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::cabinet),   cabinetRelay);
    hornAttach      = std::make_unique<juce::WebToggleButtonParameterAttachment> (*params.getParameter (ParameterIDs::horn),      hornRelay);

    // --- Int selector attachments (slider relays: combo relays need AudioParameterChoice) ---
    midFreqAttach      = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::midFreq),      midFreqRelay);
    presetSelectAttach = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::presetSelect), presetSelectRelay);
    tightAttach        = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::tight),        tightRelay);

    // --- FX Rack ---
    tightFreqAttach    = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::tightFreq),    tightFreqRelay);
    powerSoakAttach    = std::make_unique<juce::WebSliderParameterAttachment> (*params.getParameter (ParameterIDs::powerSoak),    powerSoakRelay);
}

juce::WebBrowserComponent::Options NativeBridge::buildOptions (
    const juce::WebBrowserComponent::Options& baseOptions) const
{
    // Chain all relay options onto the base options.
    // Each relay's buildOptions() registers its event listeners with the WebView.
    auto options = baseOptions
        .withNativeIntegrationEnabled()
        // Float relays
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (gainRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (tubeGainRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (bassRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (midrangeRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (trebleRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (masterRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq33Relay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq80Relay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq150Relay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq300Relay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq600Relay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq900Relay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq2kRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq5kRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geq8kRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (geqLevelRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (dryWetRelay))
        // Bool relays
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (inputPadRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (inputHiLoRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (brightRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (ultraHiRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (ultraLoRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (geqOnRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (bypassRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (muteRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (cabinetRelay))
        .withOptionsFrom (const_cast<juce::WebToggleButtonRelay&> (hornRelay))
        // Int selector relays (slider-backed)
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (midFreqRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (presetSelectRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (tightRelay))
        // FX Rack sliders
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (tightFreqRelay))
        .withOptionsFrom (const_cast<juce::WebSliderRelay&> (powerSoakRelay));

    return options;
}
