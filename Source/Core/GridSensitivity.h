#pragma once
#include <cmath>

namespace conduit::grid
{

/** Sensitivity 0..100 -> Skalierungsfaktor der Geometrie-Spanne
    (PadGridLayout::yRangeNorm, RingTouchModel-Radiusspanne). 50 -> exakt
    1.0 (heutiges Verhalten), 0 -> x0.25, 100 -> x4. Exponentiell statt
    linear, weil Empfindlichkeit multiplikativ wahrgenommen wird (gleiche
    Slider-Schritte = gleiche relative Aenderung); streng monoton, nie
    <= 0 (keine Division durch Null in expressionFromDrag/onMove).
    "Hoeher = feiner": der Aufrufer MULTIPLIZIERT die Basis-Spanne mit
    diesem Faktor (groessere Spanne = mehr Fingerweg pro Werteinheit). */
[[nodiscard]] inline float sensitivityToRangeScale (double sensitivity) noexcept
{
    return std::exp2 (static_cast<float> ((sensitivity - 50.0) / 25.0));
}

} // namespace conduit::grid
