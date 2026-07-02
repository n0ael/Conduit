#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace conduit
{

//==============================================================================
/**
    Einstellungen des OSC-Send-Pfads (CLAUDE.md 7.3) — App-Zustand, KEIN
    Patch-Zustand (eigene juce::PropertiesFile "Conduit/OscSend.settings",
    überlebt Preset-Load, kein Undo — gleiche Trennung wie MeterSettings/
    CaptureSettings).

    Ziel-Host + Port des Feedback-Streams sowie das Enable-Flag. Default-Port
    9001 (NICHT 9000 — der eigene Empfangsport, Loopback-Schutz), Enabled
    default aus.

    Threading: Setter/Getter auf dem Message Thread. UI und OscSendService
    lauschen über juce::ChangeBroadcaster.
*/
class OscSendSettings : public juce::ChangeBroadcaster
{
public:
    static constexpr int defaultPort = 9001;
    static constexpr const char* defaultHost = "127.0.0.1";

    /** Eigene Datei neben Meter.settings / ChannelNames.settings. */
    [[nodiscard]] static juce::PropertiesFile::Options defaultOptions();

    /** Tests injizieren eigene Options (Temp-Verzeichnis). */
    explicit OscSendSettings (const juce::PropertiesFile::Options& options = defaultOptions());
    ~OscSendSettings() override;

    [[nodiscard]] juce::String getHost() const noexcept { return host; }
    void setHost (const juce::String& newHost);

    [[nodiscard]] int getPort() const noexcept { return port; }
    void setPort (int newPort);

    [[nodiscard]] bool isEnabled() const noexcept { return enabled; }
    void setEnabled (bool shouldSend);

private:
    void loadFromFile();
    void store (const char* key, const juce::var& value);

    juce::ApplicationProperties applicationProperties;

    juce::String host { defaultHost };
    int port = defaultPort;
    bool enabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscSendSettings)
};

} // namespace conduit
