#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace conduit::midirig
{

//==============================================================================
/** Ein Parameter eines Klangerzeuger-Profils (ADR 006 E1, midi.guide-CSV).
    Ein Param kann CC, NRPN oder beides tragen; -1 = Adresse nicht belegt. */
struct ProfileParam
{
    juce::String section;
    juce::String name;
    int cc   = -1;    // aus cc_msb (0..127)
    int nrpn = -1;    // nrpn_msb*128 + nrpn_lsb (0..16383)
    int minValue = 0;
    int maxValue = 127;
};

/** Klangerzeuger-Profil — eine CSV-Datei je Gerät. */
struct DeviceProfile
{
    juce::String manufacturer;
    juce::String device;
    std::vector<ProfileParam> params;

    /** Anzeige-/Vergleichsschlüssel "Hersteller Gerät". */
    [[nodiscard]] juce::String displayName() const
    {
        return (manufacturer + " " + device).trim();
    }
};

//==============================================================================
/** Ergebnis eines Parse-Laufs (ADR E1b: kein stilles Scheitern —
    die UI zeigt den Report). */
struct ParseReport
{
    int accepted = 0;   // übernommene Parameter-Zeilen
    int skipped  = 0;   // verworfene Zeilen (ohne Name/Adresse, kaputt)
    juce::StringArray warnings;
};

/** Toleranter midi.guide-CSV-Parser (pure, Muster HardwareCcDatabase):
    Header-Zeilen-getrieben — Spalten werden über ihre Namen gefunden
    (case-insensitiv), unbekannte Spalten ignoriert (ADR E1). CSV-Quoting
    ("…, mit Komma …", "" als Escape) wird unterstützt. Eine Datei kann
    mehrere Geräte enthalten (Gruppierung über manufacturer+device).
    Zeilen ohne parameter_name oder ohne (cc_msb ODER nrpn_msb) werden
    gezählt und übersprungen. */
[[nodiscard]] std::vector<DeviceProfile> parseMidiGuideCsv (const juce::String& text,
                                                            ParseReport* report = nullptr);

} // namespace conduit::midirig
