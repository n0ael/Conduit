#pragma once

#include <vector>

#include <juce_graphics/juce_graphics.h>

namespace conduit::grid
{

//==============================================================================
/** Figma-Style-Snapping der freien Control-Anordnung (Block F, Masterplan):
    beim Bewegen zentriert sich ein Element an der MITTEL-Achse eines
    anderen; bei dreien wird der gleiche Abstand angeboten (Mittelpunkt
    zwischen zwei Nachbarn bzw. die Verlaengerung einer Zweierreihe);
    kleines Ausweichen ueber die Schwelle hinaus loest die Flucht wieder
    (das Snapping rechnet jede Bewegung von der ROHEN Zeigerposition aus,
    es gibt keinen Rast-Zustand). Achsen unabhaengig, Koordinaten
    normalisiert. UI-frei, testbar. */
struct FigmaSnap
{
    struct Result
    {
        float x = 0.0f, y = 0.0f;         // ggf. gesnappte Ursprungs-Position
        bool  snappedX = false, snappedY = false;
        float guideX = 0.0f, guideY = 0.0f;   // Achsen-Position (Zentrum) fuer die Hilfslinie
    };

    /** moving = Rechteck an der rohen Zielposition; others = Rechtecke der
        uebrigen Controls (gleiches Koordinatensystem); threshold = maximale
        Distanz (pro Achse), innerhalb derer eingerastet wird. */
    [[nodiscard]] static Result snap (juce::Rectangle<float> moving,
                                      const std::vector<juce::Rectangle<float>>& others,
                                      float thresholdX, float thresholdY) noexcept;

    /** Kandidaten-Zentren einer Achse: die Zentren aller anderen Elemente
        (Achs-Flucht) plus pro Paar der Mittelpunkt (gleicher Abstand
        ZWISCHEN beiden) und die beidseitigen Verlaengerungen (gleicher
        Abstand NEBEN einer Zweierreihe). Headless testbar. */
    [[nodiscard]] static std::vector<float> centreCandidates (const std::vector<float>& otherCentres);
};

} // namespace conduit::grid
