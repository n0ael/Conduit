#include "CurveEditorTile.h"

namespace conduit
{

namespace
{
    constexpr float kFieldInset      = 6.0f;
    constexpr float kEndpointRadius  = 4.0f;
    constexpr float kTouchTargetPx   = 32.0f;   // Mini-Format: kleiner als die 44er
                                                 // MPE-Kachel, Griffe bleiben greifbar
    constexpr float kCurvatureSensitivity = 1.5f;
}

CurveEditorTile::CurveEditorTile (grid::ResponseCurve& curveToEdit)
    : curve (curveToEdit)
{
    setInterceptsMouseClicks (true, false);
}

void CurveEditorTile::setAccentColour (juce::Colour newColour)
{
    if (accentColour == newColour)
        return;

    accentColour = newColour;
    repaint();
}

void CurveEditorTile::setLiveValues (float input01, float output01)
{
    liveInput01  = juce::jlimit (0.0f, 1.0f, input01);
    liveOutput01 = juce::jlimit (0.0f, 1.0f, output01);
    hasLiveValue = true;
    repaint();
}

juce::Rectangle<float> CurveEditorTile::fieldBounds() const noexcept
{
    return getLocalBounds().toFloat().reduced (kFieldInset);
}

juce::Point<float> CurveEditorTile::normalisedPosition (juce::Point<float> pos) const noexcept
{
    const auto field = fieldBounds();
    if (field.getWidth() <= 0.0f || field.getHeight() <= 0.0f)
        return {};

    return { (pos.x - field.getX()) / field.getWidth(),
             (field.getBottom() - pos.y) / field.getHeight() };   // y: 0 = unten
}

void CurveEditorTile::notifyChanged()
{
    if (onCurveChanged != nullptr)
        onCurveChanged();
}

void CurveEditorTile::paint (juce::Graphics& g)
{
    const auto tile = getLocalBounds().toFloat();
    g.setColour (push::colours::tile);
    g.fillRoundedRectangle (tile, 4.0f);
    g.setColour (push::colours::outline);
    g.drawRoundedRectangle (tile, 4.0f, 1.0f);

    const auto field = fieldBounds();
    if (field.isEmpty())
        return;

    const auto outMin = curve.getOutputMin();
    const auto outMax = curve.getOutputMax();
    const auto lo = juce::jmin (outMin, outMax);
    const auto hi = juce::jmax (outMin, outMax);

    const auto toY = [&] (float value)
    {
        return field.getBottom() - juce::jlimit (lo, hi, value) * field.getHeight();
    };

    // Diagonale Referenz (Identitaet)
    g.setColour (push::colours::outline);
    g.drawLine (field.getX(), field.getBottom(), field.getRight(), field.getY());

    // Kurvenlinie
    juce::Path path;
    for (int i = 0; i <= kCurveSamples; ++i)
    {
        const auto x = (float) i / (float) kCurveSamples;
        const auto px = field.getX() + x * field.getWidth();
        const auto py = toY (curve.apply (x));

        if (i == 0)
            path.startNewSubPath (px, py);
        else
            path.lineTo (px, py);
    }

    const auto editing = activeTarget != grid::CurveEditInteraction::Target::None;
    g.setColour (editing ? accentColour.brighter (0.4f) : accentColour);
    g.strokePath (path, juce::PathStrokeType (editing ? 2.5f : 1.5f));

    // Endpunkt-Griffe (Hoehe = tatsaechlicher outMin-/outMax-Wert)
    g.setColour (accentColour);
    g.fillEllipse (juce::Rectangle<float> (kEndpointRadius * 2.0f, kEndpointRadius * 2.0f)
                       .withCentre ({ field.getX(), toY (outMin) }));
    g.fillEllipse (juce::Rectangle<float> (kEndpointRadius * 2.0f, kEndpointRadius * 2.0f)
                       .withCentre ({ field.getRight(), toY (outMax) }));

    // Mittelpunkt-Griff (3-Punkt-Kurve): hohler Ring wie im MPE-Editor
    if (curve.numPoints() == 3)
    {
        const auto& mid = curve.points()[1];
        const auto midValue = outMin + mid.y * (outMax - outMin);
        g.drawEllipse (juce::Rectangle<float> (kEndpointRadius * 2.0f, kEndpointRadius * 2.0f)
                           .withCentre ({ field.getX() + mid.x * field.getWidth(), toY (midValue) }),
                       1.5f);
    }

    // Live-Marker: Punkt auf der Kurve am aktuellen Eingang (Compact-View).
    if (hasLiveValue)
    {
        g.setColour (push::colours::ledWhite);
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f)
                           .withCentre ({ field.getX() + liveInput01 * field.getWidth(),
                                          toY (liveOutput01) }));
    }
}

void CurveEditorTile::mouseDown (const juce::MouseEvent& event)
{
    const auto field = fieldBounds();
    if (field.isEmpty())
        return;

    const auto normPos = normalisedPosition (event.position);
    const auto hitRadiusNorm = kTouchTargetPx / juce::jmax (1.0f, field.getHeight());

    activeTarget = grid::CurveEditInteraction::hitTest (normPos, curve.getOutputMin(),
                                                        curve.getOutputMax(), hitRadiusNorm, curve);
    activeSegment = grid::CurveEditInteraction::curvatureSegmentAt (curve, normPos.x);
    startNormY = normPos.y;
    curvatureAtDown = segmentCurvature[(size_t) activeSegment];

    repaint();
}

void CurveEditorTile::mouseDrag (const juce::MouseEvent& event)
{
    if (activeTarget == grid::CurveEditInteraction::Target::None)
        return;

    const auto normPos = normalisedPosition (event.position);

    switch (activeTarget)
    {
        case grid::CurveEditInteraction::Target::MinEndpoint:
            curve.setOutputRange (grid::CurveEditInteraction::endpointValueFromY (normPos.y),
                                  curve.getOutputMax());
            break;

        case grid::CurveEditInteraction::Target::MaxEndpoint:
            curve.setOutputRange (curve.getOutputMin(),
                                  grid::CurveEditInteraction::endpointValueFromY (normPos.y));
            break;

        case grid::CurveEditInteraction::Target::MidPoint:
            grid::CurveEditInteraction::applyMidPointDrag (curve, normPos,
                                                           curve.getOutputMin(), curve.getOutputMax());
            break;

        case grid::CurveEditInteraction::Target::Curvature:
        {
            const auto newCurvature = juce::jlimit (-1.0f, 1.0f,
                curvatureAtDown + grid::CurveEditInteraction::curvatureDelta (
                    startNormY, normPos.y, kCurvatureSensitivity));
            curve.setSegmentCurvature (activeSegment, newCurvature);
            segmentCurvature[(size_t) activeSegment] = newCurvature;
            break;
        }

        case grid::CurveEditInteraction::Target::None:
        default:
            break;
    }

    notifyChanged();
    repaint();
}

void CurveEditorTile::mouseUp (const juce::MouseEvent&)
{
    activeTarget = grid::CurveEditInteraction::Target::None;
    repaint();
}

} // namespace conduit
