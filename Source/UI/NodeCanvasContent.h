#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Transform-Träger des Node-Patch-Editors (ADR 008 M3a): einzige direkte
    Kind-Component des NodeCanvas. Hält alle NodeComponents als Kinder und
    trägt den Viewport als juce::AffineTransform (Zoom + Pan) — JUCE rechnet
    Hit-Testing, Koordinaten-Konvertierung und Events der Kinder automatisch
    durch den Transform, die Modul-Interaktionen bleiben unverändert.

    - Bewusst PASSIV: setInterceptsMouseClicks(false, true) — Klicks auf den
      Hintergrund fallen zum NodeCanvas durch (Kabel-Trennen, Doppel-Tap,
      Gesten-Leiter); nur die Kinder fangen ihre Events. Die Leerraum-Regel
      (ADR 008) entsteht so per Konstruktion.
    - Interaktions-Sperre (Zoom < UiSettings::interactionMinZoom):
      setChildInteraction(false) → auch die Kinder werden durchlässig,
      Module sind reine Navigationsziele.
    - Feste, große Bounds (contentSize²): der Content-Koordinatenraum ist
      der Node-Koordinatenraum des Trees (x/y ≥ 0). JUCE prüft Events gegen
      die Bounds — deshalb groß statt „unendlich"; Kinder clippt ohnehin
      erst der Canvas.
    - Kabel zeichnet der NodeCanvas über onPaintCables in Content-Koordinaten
      (die Node-Positionen SIND Content-Koordinaten — Logik unverändert).
*/
class NodeCanvasContent final : public juce::Component
{
public:
    static constexpr int contentSize = 200000;   // 200k px je Achse (× Zoom 2 << int-Grenze)

    NodeCanvasContent()
    {
        setBounds (0, 0, contentSize, contentSize);
        setInterceptsMouseClicks (false, true);
    }

    /** Kabel-Rendering des Canvas (Content-Koordinaten, geclippter Graphics). */
    std::function<void (juce::Graphics&)> onPaintCables;

    /** Interaktions-Sperre (ADR 008 M3a): false → Kinder bekommen keine
        Events mehr, Module sind reine Navigationsziele. */
    void setChildInteraction (bool enabled)
    {
        setInterceptsMouseClicks (false, enabled);
    }

    void paint (juce::Graphics& g) override
    {
        if (onPaintCables)
            onPaintCables (g);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeCanvasContent)
};

} // namespace conduit
