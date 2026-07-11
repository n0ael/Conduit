#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/CurveEditInteraction.h"
#include "Core/ResponseCurve.h"
#include "PushLookAndFeel.h"

namespace conduit
{

//==============================================================================
/**
    Wiederverwendbarer Mini-Kurveneditor (Block E, Masterplan: "exakt die
    MPE-Editor-Optik/-Interaktion, aber ohne Offset") -- die Component-
    Extraktion der Block-C-Helfer (CurveEditInteraction) fuer das
    Macro-System. Bearbeitet eine EXTERN besessene ResponseCurve (der
    Besitzer, z. B. MacroBinding, haelt die Kurve; diese Tile ist reine
    Editier-Oberflaeche).

    Gesten (Ein-Finger, Teilmenge des MPE-Editors): Endpunkt-Griffe (Min/
    Max) ziehen, Mittelpunkt-Griff ziehen (falls 3-Punkt-Kurve -- der
    Mittelpunkt entsteht hier NICHT per Drehgeste, Default bleibt 2 Punkte),
    Kruemmungs-Wisch im freien Feld (links/rechts vom Mittelpunkt =
    Segment 0/1). Kein Offset, keine Noten-Kreise, keine Zwei-/Drei-Finger-
    Gesten (Mini-Format).

    Optional zeigt ein Marker den zuletzt gesendeten Wert (setLiveValues,
    Compact-View-Anzeige des Macro-Panels). Message Thread.
*/
class CurveEditorTile final : public juce::Component
{
public:
    explicit CurveEditorTile (grid::ResponseCurve& curveToEdit);

    void setAccentColour (juce::Colour newColour);

    /** Aktueller Eingangs-/Ausgangswert (Punkt auf der Kurve) -- NAN-frei,
        [0,1]; Anzeige nur, kein Editieren. */
    void setLiveValues (float input01, float output01);

    std::function<void()> onCurveChanged;

    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp   (const juce::MouseEvent& event) override;

    static constexpr int kCurveSamples = 32;

private:
    [[nodiscard]] juce::Rectangle<float> fieldBounds() const noexcept;
    [[nodiscard]] juce::Point<float> normalisedPosition (juce::Point<float> pos) const noexcept;

    void notifyChanged();

    grid::ResponseCurve& curve;
    juce::Colour accentColour = push::colours::ledCyan;

    grid::CurveEditInteraction::Target activeTarget = grid::CurveEditInteraction::Target::None;
    int   activeSegment    = 0;
    float startNormY       = 0.0f;
    float curvatureAtDown  = 0.0f;
    std::array<float, 2> segmentCurvature {};   // Schatten (ResponseCurve hat keinen Getter)

    float liveInput01  = 0.0f;
    float liveOutput01 = 0.0f;
    bool  hasLiveValue = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveEditorTile)
};

} // namespace conduit
