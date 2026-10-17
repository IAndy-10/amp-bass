#include "PluginEditor.h"
#include "BinaryData.h"

AmpBassAudioProcessorEditor::AmpBassAudioProcessorEditor (AmpBassAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Connect all relays to APVTS parameters (creates attachments with gesture handling)
    nativeBridge.connectToAPVTS (processorRef.apvts);

    // Build WebBrowserComponent options with all relays registered
    auto options = nativeBridge.buildOptions (
        juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withResourceProvider ([this] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
            {
                // Serve embedded binary data resources
                const auto urlToRetrieve = url.fromFirstOccurrenceOf ("://", false, false);
                const auto resource = getResource (urlToRetrieve);
                if (resource.has_value())
                    return resource;
                return std::nullopt;
            })
    );

    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (webView.get());

    // Faceplate design canvas is 2562 x 558 (UI spec §2.1): keep that aspect.
    constexpr double kAspect = 2562.0 / 558.0;
    setResizable (true, true);
    setResizeLimits (960, juce::roundToInt (960 / kAspect), 1800, juce::roundToInt (1800 / kAspect));
    getConstrainer()->setFixedAspectRatio (kAspect);
    setSize (1400, juce::roundToInt (1400 / kAspect));

    // Navigate to the embedded index.html via resource provider
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot() + "index.html");

    startTimerHz (30);
}

AmpBassAudioProcessorEditor::~AmpBassAudioProcessorEditor()
{
    stopTimer();
}

void AmpBassAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (processorRef.peakFlag.exchange (false, std::memory_order_relaxed))
        lastPeakMs = now;

    const bool lit = (now - lastPeakMs) < kPeakHoldMs;
    // Only record a state the page actually received; a hidden WebView drops
    // events, and the change is sent again once it is showing.
    if (lit != peakLit && webView != nullptr && webView->isShowing())
    {
        peakLit = lit;
        webView->emitEventIfBrowserIsVisible ("peak", lit);
    }
}

void AmpBassAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void AmpBassAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
AmpBassAudioProcessorEditor::getResource (const juce::String& url)
{
    // Map URL paths to embedded binary data
    // The Svelte build produces index.html (and potentially other assets)

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        auto resourceName = juce::String (BinaryData::namedResourceList[i]);

        // Match the URL path to a binary data resource name
        auto urlFileName = url.fromLastOccurrenceOf ("/", false, false);
        if (urlFileName.isEmpty())
            urlFileName = "index.html";

        // BinaryData names replace dots and hyphens with underscores
        auto expectedName = urlFileName.replace (".", "_").replace ("-", "_");

        if (resourceName == expectedName)
        {
            int size = 0;
            const char* data = BinaryData::getNamedResource (
                BinaryData::namedResourceList[i], size);

            if (data != nullptr)
            {
                // Determine MIME type
                juce::String mimeType = "application/octet-stream";
                if (urlFileName.endsWith (".html")) mimeType = "text/html";
                else if (urlFileName.endsWith (".js"))   mimeType = "application/javascript";
                else if (urlFileName.endsWith (".css"))  mimeType = "text/css";
                else if (urlFileName.endsWith (".svg"))  mimeType = "image/svg+xml";
                else if (urlFileName.endsWith (".png"))  mimeType = "image/png";
                else if (urlFileName.endsWith (".json")) mimeType = "application/json";

                return juce::WebBrowserComponent::Resource {
                    std::vector<std::byte> (
                        reinterpret_cast<const std::byte*> (data),
                        reinterpret_cast<const std::byte*> (data + size)),
                    mimeType.toStdString()
                };
            }
        }
    }

    return std::nullopt;
}
