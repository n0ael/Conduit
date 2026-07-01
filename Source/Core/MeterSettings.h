#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace conduit
{

//==============================================================================
/**
    Einstellungen der Pegelanzeigen — App-Zustand, KEIN Patch-Zustand
    (eigene juce::PropertiesFile "Conduit/Meter.settings", überlebt Preset-
    Load, kein Undo — gleiche Trennung wie ChannelNames/CaptureSettings).

    Aktuell: Clip-Reset-Verhalten.
      - manual    → Clip-Latch bleibt bis Klick auf das Clip-Feld
      - automatic → Latch verlischt nach autoClearSeconds von selbst
                    (Klick funktioniert zusätzlich weiterhin)

    Threading: Setter/Getter auf dem Message Thread. UI-Benachrichtigung über
    juce::ChangeBroadcaster; der EngineProcessor lauscht und speist die
    LevelMeter (setClipHoldSeconds).
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

private:
    void loadFromFile();

    juce::ApplicationProperties applicationProperties;
    ClipResetMode clipResetMode = defaultMode;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterSettings)
};

} // namespace conduit
