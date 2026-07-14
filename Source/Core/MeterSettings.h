#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "Capture/LevelMeter.h"   // Ballistik-Defaults + Klemm-Bereiche

namespace conduit
{

//==============================================================================
/**
    Einstellungen der Pegelanzeigen — App-Zustand, KEIN Patch-Zustand
    (eigene juce::PropertiesFile "Conduit/Meter.settings", überlebt Preset-
    Load, kein Undo — gleiche Trennung wie ChannelNames/CaptureSettings).

    Clip-Reset-Verhalten:
      - manual    → Clip-Latch bleibt bis Klick auf das Clip-Feld
      - automatic → Latch verlischt nach autoClearSeconds von selbst
                    (Klick funktioniert zusätzlich weiterhin)

    Ballistik (User-Feintuning 14.07.2026, Metering-Tab): RMS-Fenster,
    Peak-Release und Peak-Hold-Zeit — app-weit für ALLE Pegelanzeigen
    (LevelMeter::setGlobalBallistics), persönlicher Geschmack.

    Threading: Setter/Getter auf dem Message Thread. UI-Benachrichtigung über
    juce::ChangeBroadcaster; der EngineProcessor lauscht und speist die
    LevelMeter (setClipHoldSeconds + setGlobalBallistics).
*/
class MeterSettings : public juce::ChangeBroadcaster
{
public:
    enum class ClipResetMode { manual, automatic };

    static constexpr float autoClearSeconds   = 2.5f;
    static constexpr ClipResetMode defaultMode = ClipResetMode::manual;

    /** Eigene Datei neben Conduit.settings / ChannelNames.settings. */
    [[nodiscard]] static juce::PropertiesFile::Options defaultOptions();

    /** Tests injizieren eigene Options (Temp-Verzeichnis). */
    explicit MeterSettings (const juce::PropertiesFile::Options& options = defaultOptions());
    ~MeterSettings() override;

    [[nodiscard]] ClipResetMode getClipResetMode() const noexcept { return clipResetMode; }
    void setClipResetMode (ClipResetMode mode);

    /** Auto-Clear-Dauer für die LevelMeter: 0 = nur manuell, sonst Sekunden. */
    [[nodiscard]] float getClipHoldSeconds() const noexcept
    {
        return clipResetMode == ClipResetMode::automatic ? autoClearSeconds : 0.0f;
    }

    //==========================================================================
    // Ballistik (Metering-Tab) — geklemmt auf die UI-Bereiche unten.

    static constexpr float minRmsWindowSeconds = 0.05f,  maxRmsWindowSeconds = 1.0f;
    static constexpr float minPeakReleaseSeconds = 0.2f, maxPeakReleaseSeconds = 3.0f;
    static constexpr float minPeakHoldSeconds = 0.5f,    maxPeakHoldSeconds = 5.0f;

    [[nodiscard]] float getRmsWindowSeconds() const noexcept { return rmsWindowSeconds; }
    void setRmsWindowSeconds (float seconds);

    [[nodiscard]] float getPeakReleaseSeconds() const noexcept { return peakReleaseSeconds; }
    void setPeakReleaseSeconds (float seconds);

    [[nodiscard]] float getPeakHoldSeconds() const noexcept { return peakHoldSeconds; }
    void setPeakHoldSeconds (float seconds);

private:
    void loadFromFile();
    void storeFloat (const char* key, float value);

    juce::ApplicationProperties applicationProperties;
    ClipResetMode clipResetMode = defaultMode;
    float rmsWindowSeconds   = LevelMeter::defaultRmsWindowSeconds;
    float peakReleaseSeconds = LevelMeter::defaultPeakReleaseSeconds;
    float peakHoldSeconds    = LevelMeter::defaultPeakHoldSeconds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterSettings)
};

} // namespace conduit
