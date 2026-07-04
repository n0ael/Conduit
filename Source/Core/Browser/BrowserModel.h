#pragma once

#include <functional>
#include <vector>

#include <juce_data_structures/juce_data_structures.h>

#include "BrowserContextProvider.h"
#include "Modules/ModuleFactory.h"

namespace conduit
{

//==============================================================================
/**
    Headless-Modell des Browser-Panels: Navigationszustand + sichtbare
    Zeilen. Die UI (BrowserPanel) rendert rows() 1:1 und meldet Klicks
    zurück — Navigations-Klicks verarbeitet das Modell selbst, Aktions-
    Zeilen (Modul anlegen, Datei laden) behandelt der Besitzer über die
    Panel-Hooks.

    Zustand lebt in einem EIGENEN ValueTree ("ui.browser") — bewusst NICHT
    am Patch-Root (User-Entscheidung 04.07.2026): Navigation/Suche landen
    nie in Presets und nie in der Undo-Historie (alle setProperty mit
    nullptr-UndoManager).

    Informationsarchitektur (fix): PROJEKTE · AUDIO (Loops/One-Shots/
    Captures) · MODULE (CV/Control · AudioFX, Kategorien aus den
    ModuleDescriptors). Maximal zwei Navigationsebenen, dann flache Liste.

    Nur Message Thread.
*/
class BrowserModel final
{
public:
    /** UI-agnostische Icon-Referenz — BrowserPanel mappt auf push::Icon
        (Core kennt keine UI-Header). */
    enum class Icon { none, projects, audio, cvControl, audioFx };

    struct Row
    {
        enum class Kind
        {
            section,    // PROJEKTE / AUDIO / MODULE          (navigiert)
            branch,     // CV/Control / AudioFX                (navigiert, ab M2)
            category,   // z.B. "Reverb/Delay"                 (navigiert, ab M2)
            module,     // Modul-Eintrag, id = factoryKey      (Aktion, ab M3)
            file,       // Projekt-/Audio-Datei                (Aktion, ab M6)
            action,     // z.B. "Preset laden…"                (Aktion)
            hint        // nicht klickbarer Hinweis/Empty-State
        };

        Kind kind = Kind::hint;
        Icon icon = Icon::none;
        juce::String label;
        juce::String id;      // Section-/Branch-Name, Kategorie oder factoryKey
    };

    BrowserModel (ModuleFactory& factoryToUse, BrowserContextProvider& contextToUse);

    //==========================================================================
    // Navigation [Message Thread]

    /** Öffnet den Startbereich der aktiven Page (Panel-Öffnen). */
    void openStartSection();

    void openSection (BrowserContextProvider::Section section);

    void goBack();
    [[nodiscard]] bool canGoBack() const;

    /** Kopfzeile: "Browser" (Übersicht) bzw. Pfad "MODULE". */
    [[nodiscard]] juce::String breadcrumbText() const;

    /** Klick auf Zeile index: Navigations-Zeilen (section/branch/category)
        verarbeitet das Modell und liefert true; alles andere false —
        der Aufrufer (Panel) behandelt Aktions-Zeilen über seine Hooks. */
    bool activateRow (int index);

    [[nodiscard]] const std::vector<Row>& rows() const noexcept { return visibleRows; }

    /** Feuert nach jedem rows()-Neuaufbau. */
    std::function<void()> onRowsChanged;

    /** Eigener Zustands-Tree — NIE an den Patch-Root hängen. */
    juce::ValueTree state { "ui.browser" };

private:
    void handleContextChanged();
    void rebuildRows();

    [[nodiscard]] juce::String currentSectionName() const;
    void setCurrentSection (const juce::String& sectionName);

    ModuleFactory& factory;
    BrowserContextProvider& context;

    std::vector<Row> visibleRows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserModel)
};

//==============================================================================
/** Section ↔ String (ValueTree-Property + Row-Ids). */
[[nodiscard]] juce::String toString (BrowserContextProvider::Section section);
[[nodiscard]] BrowserContextProvider::Section sectionFromString (const juce::String& name);

} // namespace conduit
