#include "GridPhysics.h"

#include <cmath>

namespace conduit::grid
{

namespace
{
    // Interner Integrations-Teilschritt: 5 ms hält die semi-implizite
    // Euler-Feder auch bei ausgelassenen Frames (dt bis ~100 ms) stabil.
    constexpr float kMaxSubstepSeconds = 0.005f;

    // Unterhalb dieser Deltas gilt die Bewegung als eingeschwungen
    // (Pad-Breiten bzw. Pad-Breiten/s).
    constexpr float kSettlePos = 1.0e-3f;
    constexpr float kSettleVel = 1.0e-2f;

    // Schnelle Kraft-Ausblendung, wenn der Finger aktiv bewegt (Sekunden
    // von 1 → 0) — der Magnet lässt sofort spürbar los.
    constexpr float kReleaseFadeSeconds = 0.08f;
}

void stepSpring (SpringState& state, float target, const SpringParams& params,
                 float dtSeconds, float forceScale01) noexcept
{
    const auto k    = juce::jmax (0.01f, params.force);
    const auto mass = juce::jmax (0.01f, params.mass);
    const auto zeta = juce::jmap (juce::jlimit (0.0f, 1.0f, params.inertia01),
                                  0.0f, 1.0f, 1.1f, 0.12f);
    // Dämpfung aus der VOLLEN Federkonstante — forceScale skaliert nur die
    // Zielkraft, damit die Bewegung bei 0 auslaufend zur Ruhe kommt.
    const auto damping = 2.0f * zeta * std::sqrt (k * mass);
    const auto scale   = juce::jlimit (0.0f, 1.0f, forceScale01);

    auto remaining = juce::jlimit (0.0f, 0.25f, dtSeconds);

    while (remaining > 0.0f)
    {
        const auto h = juce::jmin (remaining, kMaxSubstepSeconds);
        remaining -= h;

        const auto accel = (k * scale * (target - state.pos) - damping * state.vel) / mass;
        state.vel += accel * h;
        state.pos += state.vel * h;
    }
}

//==============================================================================
void GridGravity::onDown (int fingerId, float xPads, float anchorPads)
{
    Finger finger;
    finger.touchX  = xPads;
    finger.anchorX = anchorPads;
    finger.eff.pos = xPads;

    fingers[fingerId] = finger;
}

void GridGravity::onMove (int fingerId, float xPads) noexcept
{
    const auto it = fingers.find (fingerId);
    if (it == fingers.end())
        return;

    auto& finger = it->second;
    finger.movedAccum += std::abs (xPads - finger.touchX);
    finger.touchX = xPads;

    // Solange der Magnet nie gegriffen hat, folgt der effektive Wert dem
    // Finger EXAKT (verlustfreier Bypass — kein Feder-Nachlauf beim Spielen).
    if (! finger.engaged)
        finger.eff.pos = xPads;
}

void GridGravity::onUp (int fingerId) noexcept
{
    fingers.erase (fingerId);
}

void GridGravity::clear() noexcept
{
    fingers.clear();
}

bool GridGravity::tick (float dtSeconds)
{
    if (fingers.empty() || dtSeconds <= 0.0f)
        return false;

    auto changed = false;

    for (auto& [fingerId, finger] : fingers)
    {
        juce::ignoreUnused (fingerId);

        const auto speed = finger.movedAccum / dtSeconds;
        finger.movedAccum = 0.0f;

        if (speed > config.movementThresholdPadsPerSec)
        {
            // Aktive Bewegung: Magnet lässt schnell los, Stillstands-Uhr neu.
            finger.stillSeconds = 0.0f;
            finger.fade = juce::jmax (0.0f, finger.fade - dtSeconds / kReleaseFadeSeconds);
        }
        else
        {
            finger.stillSeconds += dtSeconds;

            if (finger.stillSeconds * 1000.0f >= (float) config.delayMs)
                finger.fade = juce::jmin (1.0f,
                    finger.fade + dtSeconds * 1000.0f / (float) juce::jmax (1, config.fadeMs));
        }

        if (! finger.engaged)
        {
            if (finger.fade <= 0.0f)
                continue;   // Bypass: effektiv == Touch, nichts zu simulieren

            // Magnet greift zum ersten Mal: Feder am aktuellen Touch starten.
            finger.engaged = true;
            finger.eff.pos = finger.touchX;
            finger.eff.vel = 0.0f;
        }

        // Zielwert: Touch → nächstgelegener perfekter Pitch, überblendet mit
        // der eingefadeten Magnetkraft (fade-time-force). Der Magnet zielt
        // auf das Raster um den In-Tune-Anker (anchor + n·Pad-Breite) —
        // exakt die Positionen, an denen die Treppen-Kennlinie ganze
        // Halbtonschritte liefert.
        const auto magnet = finger.anchorX + std::round (finger.touchX - finger.anchorX);
        const auto target = finger.touchX + (magnet - finger.touchX) * finger.fade;

        const auto before = finger.eff.pos;
        stepSpring (finger.eff, target, config.spring, dtSeconds);

        if (std::abs (finger.eff.pos - before) > kSettlePos * 0.1f)
            changed = true;

        // Ausgeschwungen und Kraft weg → zurück in den exakten Bypass.
        if (finger.fade <= 0.0f
            && std::abs (finger.eff.pos - finger.touchX) < kSettlePos
            && std::abs (finger.eff.vel) < kSettleVel)
        {
            finger.engaged = false;
            finger.eff.pos = finger.touchX;
            finger.eff.vel = 0.0f;
            changed = true;
        }
    }

    return changed;
}

float GridGravity::effectiveX (int fingerId, float fallbackXPads) const noexcept
{
    const auto it = fingers.find (fingerId);
    if (it == fingers.end() || ! it->second.engaged)
        return fallbackXPads;

    return it->second.eff.pos;
}

float GridGravity::pullAmount (int fingerId) const noexcept
{
    const auto it = fingers.find (fingerId);
    return it != fingers.end() ? it->second.fade : 0.0f;
}

} // namespace conduit::grid
