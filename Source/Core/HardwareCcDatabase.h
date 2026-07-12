#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace conduit::grid
{

//==============================================================================
/** Ein CC-Parameter eines Hardware-Geräts (Block L2, Masterplan). */
struct HardwareCcParam
{
    int cc = 0;
    juce::String name;
};

/** Ein Hardware-Gerät mit seinen bekannten CC-Parametern. */
struct HardwareDevice
{
    juce::String name;
    std::vector<HardwareCcParam> params;
};

//==============================================================================
/**
    Hardware-CC-Datenbank (Block L2, Masterplan — Nachfolgeschritt zur
    generischen CcNames-Tabelle aus Block L): "welche CC-Nummer heißt bei
    Gerät X wie" — damit der Macro-Panel-Ziel-Typ „Hardware" Device →
    Parameter anbietet statt roher CC-Zahlen (analog zum Ableton-
    Parameter-Browser, Track → Device → Parameter).

    Faktor-Daten kommen aus BinaryData (Assets/HardwareDevices.txt, im
    Programm eingebaut); eine optionale, GLEICH strukturierte User-Datei
    (Conduit-Einstellungsordner, neben GridSession.xml) ergänzt fehlende
    Geräte oder überschreibt einzelne Parameter — gleicher Gerätename =
    dasselbe Gerät, gleiche CC-Nummer = überschreiben, neue Nummer =
    ergänzen (merge()).

    Klartext-Format statt XML (bewusst leicht von Hand editierbar,
    User-Entscheidung 12.07.2026): Kommentarzeilen mit „#", „[Gerätename]"
    eröffnet einen Block, darin „CC = Name" je Zeile. parse()/merge() sind
    pure Funktionen, UI-frei, headless testbar. Message Thread.
*/
class HardwareCcDatabase
{
public:
    /** Faktor-Daten (BinaryData) + optionale User-Datei laden und
        zusammenführen. userFile == File() (Default) = nur Faktor-Daten. */
    void load (const juce::File& userFile = {});

    [[nodiscard]] const std::vector<HardwareDevice>& devices() const noexcept { return merged; }
    [[nodiscard]] const HardwareDevice* findDevice (const juce::String& name) const noexcept;

    /** Parst den Klartext in eine Geräteliste — pure, testbar ohne
        Datei-I/O. Zeilen ohne erkennbares Format werden ignoriert
        (fehlertolerant statt abbrechend, User editiert von Hand). */
    [[nodiscard]] static std::vector<HardwareDevice> parse (const juce::String& text);

    /** overlay-Geräte/-Parameter gewinnen bei gleichem Namen bzw. gleicher
        CC-Nummer; neue Geräte/Parameter werden ergänzt. Pure, testbar. */
    [[nodiscard]] static std::vector<HardwareDevice> merge (
        std::vector<HardwareDevice> base, const std::vector<HardwareDevice>& overlay);

private:
    std::vector<HardwareDevice> merged;
};

} // namespace conduit::grid
