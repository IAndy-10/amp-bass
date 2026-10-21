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
            .withNativeFunction ("showParameterMenu", [this] (const juce::Array<juce::var>& args, auto completion)
            {
                showParameterMenu (args, std::move (completion));
            })
            // The system clipboard, which the WebView may block for the page.
            .withNativeFunction ("copyToClipboard", [] (const juce::Array<juce::var>& args, auto completion)
            {
                juce::SystemClipboard::copyTextToClipboard (args[0].toString());
                completion (true);
            })
            .withNativeFunction ("pasteFromClipboard", [] (const juce::Array<juce::var>&, auto completion)
            {
                completion (juce::SystemClipboard::getTextFromClipboard());
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

// args: parameter ID, labels of the page's own items. When the host offers a menu for the parameter
// (VST3 hosts, e.g. Ableton's "Show Automation"), shows the host's items and the page's in one menu and
// completes with the index of the page's item picked (-1 = none, or a host item). Completes with false
// when there is no host menu (AU, Standalone, older hosts); the page then shows its own menu.
void AmpBassAudioProcessorEditor::showParameterMenu (const juce::Array<juce::var>& args,
                                                     juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    auto* context   = getHostContext();
    auto* parameter = processorRef.apvts.getParameter (args[0].toString());
    auto hostMenu   = context != nullptr && parameter != nullptr ? context->getContextMenuForParameter (parameter)
                                                                 : nullptr;
    if (hostMenu == nullptr)
    {
        completion (false);
        return;
    }

    // Host items run their own actions (item ID -1); the page's items get IDs 1, 2, ...
    auto menu = hostMenu->getEquivalentPopupMenu();
    if (const auto* labels = args[1].getArray(); labels != nullptr && ! labels->isEmpty())
    {
        if (menu.getNumItems() > 0)
            menu.addSeparator();
        for (int i = 0; i < labels->size(); ++i)
            menu.addItem (i + 1, (*labels)[i].toString());
    }

    menu.showMenuAsync (juce::PopupMenu::Options{}.withTargetComponent (webView.get()).withMousePosition(),
                        [safeThis = juce::Component::SafePointer<AmpBassAudioProcessorEditor> (this),
                         completion = std::move (completion)] (int result)
                        {
                            // The page is gone with the editor; nothing to answer.
                            if (safeThis != nullptr)
                                completion (result > 0 ? result - 1 : -1);
                        });
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
