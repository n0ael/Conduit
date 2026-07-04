#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Browser/BrowserModel.h"

namespace conduit
{

//==============================================================================
/**
    Wiederverwendete Zeilen-Komponente der Browser-Liste (ListBox-
    Virtualisierung via refreshComponentForRow — nie eine Komponente pro
    Eintrag). Layout: Icon links im 24-px-Raster, Label rechts, navigier-
    bare Zeilen mit PERMANENTEM ▸ (keine Hover-Affordance, Touch!),
    Kategorien eingerückt unter ihren Ast-Headern.

    Icons werden nur für sichtbare Zeilen gezeichnet (Lazy — die
    Virtualisierung hält höchstens eine Bildschirmseite an Komponenten).

    Gesten: Tap (< Bewegungsschwelle) aktiviert die Zeile; vertikale
    Bewegungen gehören dem Viewport (Flick-Scroll, ScrollOnDragMode).
    Drag-to-Graph für Modul-Zeilen folgt in M3.
*/
class BrowserListRow final : public juce::Component
{
public:
    BrowserListRow();

    /** Von refreshComponentForRow bei jeder Wiederverwendung gerufen. */
    void update (const BrowserModel::Row& rowToShow, int rowIndexToUse,
                 bool isSelected);

    /** Tap auf die Zeile (Index aus dem letzten update()). */
    std::function<void (int rowIndex)> onActivated;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    /** Bewegungsschwelle in px: darunter Tap, darüber Scroll/Drag. */
    static constexpr int tapThreshold = 8;

private:
    BrowserModel::Row row;
    int rowIndex = -1;
    bool selected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserListRow)
};

} // namespace conduit
