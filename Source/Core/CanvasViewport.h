#pragma once

#include <cmath>

#include <juce_graphics/juce_graphics.h>

namespace conduit
{

//==============================================================================
/**
    Reine Viewport-Mathematik des Node-Patch-Editors (ADR 008 M3a) —
    headless testbar, keine Component-Abhängigkeit.

    Konvention: Der Canvas zeigt den Content durch eine affine Abbildung
      screen = content * zoom + offset
    offset/zoom leben pro Seite im ValueTree (viewOffsetX/Y, viewZoom, M1).
    Zoom-Clamps 0.1–2.0 (User-Entscheidung 18.07.2026): Untergrenze lässt
    die M4-Birdeye-Pegel zu, Obergrenze 200 % für Detailarbeit.
*/
namespace canvas_view
{
    inline constexpr double minZoom = 0.1;
    inline constexpr double maxZoom = 2.0;

    struct ViewState
    {
        double offsetX = 0.0;   // Screen-Position des Content-Ursprungs
        double offsetY = 0.0;
        double zoom    = 1.0;
    };

    [[nodiscard]] inline double clampZoom (double zoom) noexcept
    {
        return juce::jlimit (minZoom, maxZoom, zoom);
    }

    /** Content-Punkt unter einem Screen-Punkt (inverse Abbildung). */
    [[nodiscard]] inline juce::Point<double> toContent (const ViewState& view,
                                                        juce::Point<double> screenPoint) noexcept
    {
        return { (screenPoint.x - view.offsetX) / view.zoom,
                 (screenPoint.y - view.offsetY) / view.zoom };
    }

    /** Screen-Punkt eines Content-Punkts. */
    [[nodiscard]] inline juce::Point<double> toScreen (const ViewState& view,
                                                       juce::Point<double> contentPoint) noexcept
    {
        return { contentPoint.x * view.zoom + view.offsetX,
                 contentPoint.y * view.zoom + view.offsetY };
    }

    /** Zoomt auf newZoom, wobei der Content-Punkt unter dem Screen-Anker
        fix bleibt (Pinch-Zentroid bzw. Mauszeiger). */
    [[nodiscard]] inline ViewState zoomAbout (const ViewState& view,
                                              juce::Point<double> screenAnchor,
                                              double newZoom) noexcept
    {
        const auto clamped = clampZoom (newZoom);
        const auto anchorInContent = toContent (view, screenAnchor);

        return { screenAnchor.x - anchorInContent.x * clamped,
                 screenAnchor.y - anchorInContent.y * clamped,
                 clamped };
    }

    /** Weiche Zoom-Dead-Zone (User-Feedback 18.07.2026): logDelta ist die
        AKKUMULIERTE Log-Spread-Änderung seit Gestenbeginn. Innerhalb der
        Dead-Zone → 0 (reines Pan, Zoom bleibt exakt stehen); zwischen
        deadZone und fullResponse blendet ein Smoothstep den Zoom weich ein
        (kein Sprung, keine harte Schwelle); darüber volle Durchleitung. */
    [[nodiscard]] inline double softZoomResponse (double logDelta,
                                                  double deadZone = 0.06,
                                                  double fullResponse = 0.18) noexcept
    {
        const auto magnitude = std::abs (logDelta);

        if (magnitude <= deadZone)
            return 0.0;

        if (fullResponse <= deadZone)   // Dead-Zone 0/degeneriert → direkte Antwort
            return logDelta;

        const auto t = juce::jlimit (0.0, 1.0,
                                     (magnitude - deadZone) / (fullResponse - deadZone));
        return logDelta * (t * t * (3.0 - 2.0 * t));   // smoothstep-Gewicht
    }

    /** Progressive Zoom-Kurve (User-Feedback 18.07.2026): dämpft die
        akkumulierte Log-Auslenkung nichtlinear — exponent > 1 lässt den
        Zoom langsam beginnen und kontinuierlich stärker werden, gain < 1
        senkt die Gesamt-Geschwindigkeit. (1.0, 1.0) = neutral/linear.
        Läuft NACH softZoomResponse (Dead-Zone schneidet erst das
        Sensor-Rauschen, die Kurve formt dann die Antwort). */
    [[nodiscard]] inline double progressiveZoomResponse (double logDelta,
                                                         double gain,
                                                         double exponent) noexcept
    {
        const auto magnitude = std::abs (logDelta);

        if (magnitude <= 0.0)
            return 0.0;

        return (logDelta < 0.0 ? -1.0 : 1.0) * gain * std::pow (magnitude, exponent);
    }

    /** Ein Pinch-/Pan-Inkrement (Ebene 2): erst Zoom um den Anker, dann
        Pan um das Screen-Delta. scaleFactor 1.0 = reines Pan. */
    [[nodiscard]] inline ViewState applyPinch (const ViewState& view,
                                               double scaleFactor,
                                               juce::Point<double> panDelta,
                                               juce::Point<double> screenAnchor) noexcept
    {
        auto next = zoomAbout (view, screenAnchor, view.zoom * scaleFactor);
        next.offsetX += panDelta.x;
        next.offsetY += panDelta.y;
        return next;
    }
} // namespace canvas_view

} // namespace conduit
