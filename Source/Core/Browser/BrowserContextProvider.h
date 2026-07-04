#pragma once

#include <functional>

#include <juce_core/juce_core.h>

namespace conduit
{

//==============================================================================
/**
    Kontext-Quelle des Browser-Panels: leitet aus der aktiven Page ab,
    welche Bereiche sichtbar sind und in welchem Bereich das Panel öffnet.

    DIE eine Stelle für das Page→Bereich-Mapping (User-Entscheidung
    04.07.2026: „welcher Bereich sich wann öffnet, bestimme ich später,
    wenn die anderen Pages da sind" — Änderungen passieren nur hier).

    Page-Indizes == TransportBar::PageIndex (Grid 0, Mixer 1, Clip 2,
    Device 3, Looper 4). Gefüttert wird der Provider vom EngineEditor
    (selectPage-Helper — jeder Page-Wechsel läuft darüber).

    Nur Message Thread.
*/
class BrowserContextProvider final
{
public:
    enum class Section { projects, audio, modules };

    BrowserContextProvider() = default;

    /** [Message Thread] Vom Editor bei jedem Page-Wechsel aufgerufen. */
    void setActivePage (int pageIndex);

    [[nodiscard]] int getActivePage() const noexcept { return activePage; }

    /** MODULE nur in der Patch-Umgebung (Device-Page); PROJEKTE und
        AUDIO sind global sichtbar. */
    [[nodiscard]] bool isSectionVisible (Section section) const;

    /** Startbereich beim Öffnen des Panels auf der aktuellen Page —
        Device öffnet direkt in MODULE (Zurück-Pfeil führt zur
        Bereichsübersicht), alle anderen Pages in PROJEKTE. */
    [[nodiscard]] Section startSection() const;

    /** Feuert nach jedem Page-Wechsel (auch ohne Sichtbarkeits-Delta —
        der Konsument entscheidet, was neu aufgebaut wird). */
    std::function<void()> onContextChanged;

private:
    int activePage = 3;  // TransportBar::pageDevice

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserContextProvider)
};

} // namespace conduit
