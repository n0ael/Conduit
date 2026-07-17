#pragma once

#include <map>
#include <vector>

#include <juce_data_structures/juce_data_structures.h>

namespace conduit
{

//==============================================================================
/**
    Preset-Namen-Cache pro Rig-Geraet (MIDI-Rig M9b, ADR 007).

    Die Namen kommen aus dem SysEx-Preset-Scan (HardwarePresetScanner) und
    werden pro RigDevice-Uuid als XML persistiert:
    `Conduit/Devices/Presets/<uuid>.xml` (Nachbar der Profil-Ordner,
    folgt Test-Redirects ueber die injizierte Basis). Preset-Namen sind
    GERAETE-Content — die RigDevice-Uuid ist selbst persistent, die
    Laufzeit-ID-Regel (CLAUDE.md 6) ist nicht beruehrt.

    Invalidierung NUR manuell (Rescan) — Conduit kann Aenderungen am
    Geraet nicht erkennen. ChangeBroadcaster fuer UI-Refreshes.
    Alle Methoden Message Thread.
*/
class HardwarePresetLibrary final : public juce::ChangeBroadcaster
{
public:
    struct DevicePresets
    {
        juce::uint8 deviceIdByte = 0;          // SysEx-Device-ID (Dump-Requests)
        int programsPerBank = 0;
        std::vector<juce::String> names;       // Index = bank * programsPerBank + program
    };

    /** presetFolder darf fehlen — wird erst beim ersten Speichern angelegt. */
    explicit HardwarePresetLibrary (const juce::File& presetFolderToUse);

    /** Alle <uuid>.xml des Ordners (neu) laden. [Message Thread] */
    void reload();

    /** Scan-Ergebnis uebernehmen + persistieren + Change-Broadcast. */
    void setPresets (const juce::Uuid& deviceId, DevicePresets presets);

    /** Cache eines Geraets verwerfen (Datei loeschen) + Broadcast. */
    void clearPresets (const juce::Uuid& deviceId);

    [[nodiscard]] bool hasPresets (const juce::Uuid& deviceId) const;

    /** Leerer String, wenn unbekannt/ausserhalb. */
    [[nodiscard]] juce::String presetName (const juce::Uuid& deviceId,
                                           int bank, int program) const;

    /** Anzahl Baenke (0 ohne Cache). */
    [[nodiscard]] int bankCount (const juce::Uuid& deviceId) const;
    [[nodiscard]] int programsPerBank (const juce::Uuid& deviceId) const;

    /** SysEx-Device-ID aus dem letzten Scan (0 ohne Cache). */
    [[nodiscard]] juce::uint8 deviceIdByte (const juce::Uuid& deviceId) const;

private:
    [[nodiscard]] juce::File fileFor (const juce::Uuid& deviceId) const;

    juce::File presetFolder;
    std::map<juce::Uuid, DevicePresets> cache;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardwarePresetLibrary)
};

} // namespace conduit
