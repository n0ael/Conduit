#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "ControllerProfile.h"

namespace conduit
{

//==============================================================================
/**
    Bibliothek der Controller-Profile (ADR 006 E2): EIN Factory-Profil
    (aktuell Allen & Heath Xone:K1, direkt per BinaryData-Symbol
    referenziert -- anders als `MidiProfileLibrary`, die alle `.csv`-
    Ressourcen generisch durchsucht, wuerde ein generischer Scan hier mit
    den Klangerzeuger-CSVs im selben flachen BinaryData-Namensraum
    kollidieren, da `originalFilenames` keinen Ordnerpfad traegt) plus
    User-Ordner `Conduit/Controllers/*.csv` (FLACH, nicht rekursiv --
    ADR E2: eine Datei = ein Geraet, keine `{Hersteller}/`-Verschachtelung
    wie bei den Klangerzeuger-Profilen).

    Traegt eine Datei kein `device`-Feld, wird der Dateiname (ohne
    Endung) als Geraetename verwendet. Pro Quelle ein SourceReport fuer
    die UI (E1b-Prinzip). Message Thread.
*/
class ControllerProfileLibrary
{
public:
    struct SourceReport
    {
        juce::String sourceName;
        bool fromUserFolder = false;
        midirig::ControllerParseReport parse;
        int controls = 0;   // Anzahl geparster Controls dieser Quelle
    };

    /** userFolder = Conduit/Controllers (darf fehlen -- wird NICHT
        angelegt, User-Hoheit); File() = nur das Factory-Profil. */
    explicit ControllerProfileLibrary (const juce::File& userFolderToUse = {});

    /** [Message Thread] Factory + User-Ordner neu einlesen. */
    void reload();

    [[nodiscard]] const std::vector<midirig::ControllerProfile>& profiles() const noexcept
    {
        return loadedProfiles;
    }

    /** nullptr, wenn kein Profil mit diesem Geraetenamen existiert. */
    [[nodiscard]] const midirig::ControllerProfile* find (const juce::String& device) const noexcept;

    [[nodiscard]] const std::vector<SourceReport>& report() const noexcept { return sourceReports; }

    [[nodiscard]] juce::File userFolder() const noexcept { return userProfileFolder; }

private:
    void addProfile (midirig::ControllerProfile profile, bool fromUser);

    juce::File userProfileFolder;
    std::vector<midirig::ControllerProfile> loadedProfiles;
    std::vector<SourceReport> sourceReports;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllerProfileLibrary)
};

} // namespace conduit
