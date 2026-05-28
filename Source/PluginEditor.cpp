#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstddef>

namespace
{
    constexpr int editorWidth = 900;
    constexpr int editorHeight = 620;

    juce::String stripQueryAndFragment (juce::String path)
    {
        path = path.upToFirstOccurrenceOf ("?", false, false);
        path = path.upToFirstOccurrenceOf ("#", false, false);
        return path;
    }
}

bool ShearWebBrowser::pageAboutToLoad (const juce::String& newURL)
{
    const auto& root = juce::WebBrowserComponent::getResourceProviderRoot();
    return newURL == root || newURL.startsWith (root);
}

ShearAudioProcessorEditor::ShearAudioProcessorEditor (ShearAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      webComponent (createWebOptions()),
      inputAttachment (parameter ("input"), inputRelay),
      driveAttachment (parameter ("drive"), driveRelay),
      toneAttachment (parameter ("tone"), toneRelay),
      biasAttachment (parameter ("bias"), biasRelay),
      mixAttachment (parameter ("mix"), mixRelay),
      outputAttachment (parameter ("output"), outputRelay),
      modeAttachment (parameter ("mode"), modeRelay),
      hqAttachment (parameter ("hq"), hqRelay)
{
    addAndMakeVisible (webComponent);
    webComponent.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (false, false);
    setSize (editorWidth, editorHeight);
    startTimerHz (30);
}

ShearAudioProcessorEditor::~ShearAudioProcessorEditor()
{
    stopTimer();
}

void ShearAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff080907));
}

void ShearAudioProcessorEditor::resized()
{
    webComponent.setBounds (getLocalBounds());
}

int ShearAudioProcessorEditor::getControlParameterIndex (juce::Component&)
{
    return controlParameterIndexReceiver.getControlParameterIndex();
}

juce::WebBrowserComponent::Options ShearAudioProcessorEditor::createWebOptions()
{
    auto options = juce::WebBrowserComponent::Options {};

   #if JUCE_WINDOWS
    options = options.withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                     .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2 {}
                         .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                                  .getChildFile ("ShearWebView2"))
                         .withStatusBarDisabled()
                         .withBuiltInErrorPageDisabled()
                         .withBackgroundColour (juce::Colour (0xff080907)));
   #endif

    return options.withNativeIntegrationEnabled()
                  .withOptionsFrom (inputRelay)
                  .withOptionsFrom (driveRelay)
                  .withOptionsFrom (toneRelay)
                  .withOptionsFrom (biasRelay)
                  .withOptionsFrom (mixRelay)
                  .withOptionsFrom (outputRelay)
                  .withOptionsFrom (modeRelay)
                  .withOptionsFrom (hqRelay)
                  .withOptionsFrom (controlParameterIndexReceiver)
                  .withResourceProvider ([this] (const auto& url)
                                         {
                                             return getResource (url);
                                         });
}

std::optional<ShearAudioProcessorEditor::WebResource> ShearAudioProcessorEditor::getResource (const juce::String& url) const
{
    auto requestPath = url == "/" ? juce::String { "index.html" }
                                  : url.fromFirstOccurrenceOf ("/", false, false);

    requestPath = stripQueryAndFragment (requestPath);

    if (requestPath.isEmpty())
        requestPath = "index.html";

    if (requestPath.contains ("..") || requestPath.startsWithChar ('/'))
        return std::nullopt;

    auto file = getWebUiDistDirectory().getChildFile (requestPath);

    if (! file.existsAsFile())
        return std::nullopt;

    return WebResource { readFileAsBytes (file),
                         juce::String { getMimeForExtension (file.getFileExtension().fromFirstOccurrenceOf (".", false, false)) } };
}

void ShearAudioProcessorEditor::timerCallback()
{
    juce::DynamicObject::Ptr payload { new juce::DynamicObject() };
    payload->setProperty ("input", audioProcessor.getInputLevel());
    payload->setProperty ("output", audioProcessor.getOutputLevel());

    webComponent.emitEventIfBrowserIsVisible ("levels", juce::var { payload.get() });
}

juce::File ShearAudioProcessorEditor::getWebUiDistDirectory()
{
    const auto sourceRelative = juce::File::createFileWithoutCheckingPath (__FILE__)
                                    .getParentDirectory()
                                    .getParentDirectory()
                                    .getChildFile ("web-ui")
                                    .getChildFile ("dist");

    if (sourceRelative.isDirectory())
        return sourceRelative;

    return juce::File::getCurrentWorkingDirectory().getChildFile ("web-ui").getChildFile ("dist");
}

const char* ShearAudioProcessorEditor::getMimeForExtension (const juce::String& extension)
{
    const auto ext = extension.toLowerCase();

    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "js" || ext == "mjs")   return "text/javascript";
    if (ext == "css")                  return "text/css";
    if (ext == "json" || ext == "map") return "application/json";
    if (ext == "svg")                  return "image/svg+xml";
    if (ext == "png")                  return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "ico")                  return "image/vnd.microsoft.icon";
    if (ext == "woff2")                return "font/woff2";

    return "application/octet-stream";
}

std::vector<std::byte> ShearAudioProcessorEditor::readFileAsBytes (const juce::File& file)
{
    juce::FileInputStream stream { file };

    if (! stream.openedOk())
        return {};

    std::vector<std::byte> data (static_cast<size_t> (stream.getTotalLength()));
    stream.read (data.data(), static_cast<int> (data.size()));
    return data;
}

juce::RangedAudioParameter& ShearAudioProcessorEditor::parameter (const juce::String& parameterID) const
{
    auto* value = audioProcessor.getValueTreeState().getParameter (parameterID);
    jassert (value != nullptr);
    return *value;
}
