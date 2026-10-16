#pragma once

#include "PluginProcessor.h"
#include "NativeBridge.h"
#include <juce_gui_extra/juce_gui_extra.h>

class AmpBassAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit AmpBassAudioProcessorEditor (AmpBassAudioProcessor&);
    ~AmpBassAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    AmpBassAudioProcessor& processorRef;

    // PEAK LED: held lit for kPeakHoldMs after the last overload so short
    // peaks stay visible. Sent to JS as the "peak" event only on change.
    static constexpr double kPeakHoldMs = 300.0;
    double lastPeakMs = -1.0e9;
    bool   peakLit    = false;

    NativeBridge nativeBridge;
    std::unique_ptr<juce::WebBrowserComponent> webView;

    static std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpBassAudioProcessorEditor)
};
