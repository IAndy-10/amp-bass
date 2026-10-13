#pragma once

#include "PluginProcessor.h"
#include "NativeBridge.h"
#include <juce_gui_extra/juce_gui_extra.h>

class AmpBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AmpBassAudioProcessorEditor (AmpBassAudioProcessor&);
    ~AmpBassAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AmpBassAudioProcessor& processorRef;

    NativeBridge nativeBridge;
    std::unique_ptr<juce::WebBrowserComponent> webView;

    static std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpBassAudioProcessorEditor)
};
