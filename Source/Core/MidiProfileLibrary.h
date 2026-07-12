#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "MidiDeviceProfile.h"

namespace conduit
{

//==============================================================================
/**
    Bibliothek der Klangerzeuger-Profile (ADR 006 E1): Factory-CSVs aus
    BinaryData (Assets/DeviceProfiles, midi.guide-Daten CC-BY-SA 4.0)
    plus User-Ordner `Conduit/Devices` (alle .csv rekursiv,
    Konvention {Hersteller}/{Gerät}.csv). Ein User-Profil mit gleichem
    manufacturer+device ERSETZT das Factory-Profil komplett — Profil-
    Dateien sind User-Hoheit (Rule midirig), gemerged wird nie stumm.

    Pro Quelle entsteht ein SourceReport (E1b: kein stilles Scheitern) —
    die MIDI-Settings-UI zeigt die Liste. Message Thread.
*/
class MidiProfileLibrary
{
public:
    struct SourceReport
    {
        juce::String sourceName;   // Dateiname bzw. BinaryData-Ressource
        bool fromUserFolder = false;
        midirig::ParseReport parse;
        int devices = 0;           // Geräte aus dieser Quelle
    };

    /** userFolder = Conduit/Devices (darf fehlen — wird NICHT angelegt,
        User-Hoheit); File() = nur Factory-Profile. */
    explicit MidiProfileLibrary (const juce::File& userFolderToUse = {});

    /** [Message Thread] Factory + User-Ordner neu einlesen. */
    void reload();

    [[nodiscard]] const std::vector<midirig::DeviceProfile>& profiles() const noexcept
    {
        return loadedProfiles;
    }

    /** nullptr, wenn kein Profil "manufacturer device" existiert. */
    [[nodiscard]] const midirig::DeviceProfile* find (const juce::String& manufacturer,
                                                      const juce::String& device) const noexcept;

    [[nodiscard]] const std::vector<SourceReport>& report() const noexcept { return sourceReports; }

    [[nodiscard]] juce::File userFolder() const noexcept { return userProfileFolder; }

private:
    void addProfiles (std::vector<midirig::DeviceProfile> incoming, bool fromUser);

    juce::File userProfileFolder;
    std::vector<midirig::DeviceProfile> loadedProfiles;
    std::vector<SourceReport> sourceReports;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiProfileLibrary)
};

} // namespace conduit
