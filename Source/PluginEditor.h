#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ShearWebBrowser final : public juce::WebBrowserComponent
{
public:
    using juce::WebBrowserComponent::WebBrowserComponent;

    bool pageAboutToLoad (const juce::String& newURL) override;
};

class ShearAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit ShearAudioProcessorEditor (ShearAudioProcessor&);
    ~ShearAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    int getControlParameterIndex (juce::Component&) override;

private:
    using WebResource = juce::WebBrowserComponent::Resource;

    juce::WebBrowserComponent::Options createWebOptions();
    std::optional<WebResource> getResource (const juce::String& url) const;
    void timerCallback() override;

    static juce::File getWebUiDistDirectory();
    static const char* getMimeForExtension (const juce::String& extension);
    static std::vector<std::byte> readFileAsBytes (const juce::File& file);

    juce::RangedAudioParameter& parameter (const juce::String& parameterID) const;

    ShearAudioProcessor& audioProcessor;

    juce::WebSliderRelay inputRelay { "input" };
    juce::WebSliderRelay driveRelay { "drive" };
    juce::WebSliderRelay toneRelay { "tone" };
    juce::WebSliderRelay biasRelay { "bias" };
    juce::WebSliderRelay mixRelay { "mix" };
    juce::WebSliderRelay outputRelay { "output" };
    juce::WebComboBoxRelay modeRelay { "mode" };
    juce::WebToggleButtonRelay hqRelay { "hq" };
    juce::WebControlParameterIndexReceiver controlParameterIndexReceiver;

    ShearWebBrowser webComponent;

    juce::WebSliderParameterAttachment inputAttachment;
    juce::WebSliderParameterAttachment driveAttachment;
    juce::WebSliderParameterAttachment toneAttachment;
    juce::WebSliderParameterAttachment biasAttachment;
    juce::WebSliderParameterAttachment mixAttachment;
    juce::WebSliderParameterAttachment outputAttachment;
    juce::WebComboBoxParameterAttachment modeAttachment;
    juce::WebToggleButtonParameterAttachment hqAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShearAudioProcessorEditor)
};
