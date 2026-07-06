#include "GridKeyboardComponent.h"

#include "PushLookAndFeel.h"

namespace conduit
{

GridKeyboardComponent::GridKeyboardComponent (grid::GridVoiceEngine& engineToUse,
                                               const grid::PadGridLayout::Config& layoutConfig)
    : engine (engineToUse), layout (layoutConfig)
{
    setWantsKeyboardFocus (false);
    setInterceptsMouseClicks (true, false);
}

juce::Point<float> GridKeyboardComponent::normalisedPosition (const juce::MouseEvent& event) const noexcept
{
    const auto bounds = getLocalBounds().toFloat();
    if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
        return {};

    return { event.position.x / bounds.getWidth(), event.position.y / bounds.getHeight() };
}

int GridKeyboardComponent::fingerIdFor (const juce::MouseEvent& event) noexcept
{
    // VoiceAllocator reserviert 0 als frei-Sentinel — Touch-Source-Indizes
    // sind 0-basiert, daher +1.
    return event.source.getIndex() + 1;
}

void GridKeyboardComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto fingerId = fingerIdFor (event);
    const auto downResult = ring.onDown (static_cast<uint32_t> (fingerId), event.position);

    if (downResult.kind == grid::RingTouchModel::TouchKind::Ring)
    {
        const auto moveResult = ring.onMove (static_cast<uint32_t> (fingerId), event.position);
        if (moveResult.hasSlide)
            engine.setSlide (moveResult.owner, moveResult.slide01);

        repaint();
        return;
    }

    const auto pos = normalisedPosition (event);
    const auto pad = layout.padIndexAt (pos.x, pos.y);

    if (pad < 0)
        return;

    fingers[fingerId] = { pos.x, pad, event.position };

    // MPE-Member-Kanäle sind gepoolt (VoiceAllocator) und behalten Bend/
    // Pressure vom LETZTEN Voice-Nutzer, bis die neue Note etwas Eigenes
    // sendet — ohne expliziten Startwert läse das Instrument beim ersten
    // Ton also einen zufälligen Alt-Zustand statt 0/Ist-Position (Fund
    // 06.07.2026: Pressure "mal 0, mal 50%" direkt nach dem Touch).
    engine.noteOn (static_cast<uint32_t> (fingerId), layout.noteForPad (pad), 100);
    engine.setPitchBend (static_cast<uint32_t> (fingerId), 0.0f);
    engine.setPressure (static_cast<uint32_t> (fingerId), layout.expressionInPad (pad, pos.y));
    repaint();
}

void GridKeyboardComponent::mouseDrag (const juce::MouseEvent& event)
{
    const auto fingerId = fingerIdFor (event);
    const auto moveResult = ring.onMove (static_cast<uint32_t> (fingerId), event.position);

    if (moveResult.hasSlide)
    {
        engine.setSlide (moveResult.owner, moveResult.slide01);
        repaint();
        return;
    }

    const auto it = fingers.find (fingerId);
    if (it == fingers.end())
        return;

    const auto pos = normalisedPosition (event);

    engine.setPitchBend (static_cast<uint32_t> (fingerId),
                          layout.pitchBendSemitones (it->second.startNormX, pos.x));
    engine.setPressure (static_cast<uint32_t> (fingerId),
                         layout.expressionInPad (it->second.padIndex, pos.y));
    it->second.currentPos = event.position;
    repaint();
}

void GridKeyboardComponent::mouseUp (const juce::MouseEvent& event)
{
    const auto fingerId = fingerIdFor (event);
    const auto upResult = ring.onUp (static_cast<uint32_t> (fingerId));

    if (upResult.wasRing)
    {
        // Mond-Orbit (User-Entscheidung 06.07.2026): kein Reset-Slide mehr --
        // der letzte gesendete CC74-Wert bleibt am Instrument stehen, der
        // Kreis bleibt sichtbar eingefroren (RingTouchModel::onUp), bis ein
        // neuer Touch die Umlaufbahn wieder aufgreift.
        repaint();
        return;
    }

    if (upResult.wasPrimary)
    {
        fingers.erase (fingerId);
        engine.noteOff (static_cast<uint32_t> (upResult.primaryFinger), 0);
        repaint();
    }
}

void GridKeyboardComponent::paint (juce::Graphics& g)
{
    g.fillAll (push::colours::background);

    const auto bounds = getLocalBounds().toFloat();
    const auto cols = layout.cols();
    const auto rows = layout.rows();
    if (cols <= 0 || rows <= 0)
        return;

    const auto padWidth  = bounds.getWidth()  / (float) cols;
    const auto padHeight = bounds.getHeight() / (float) rows;
    constexpr float gap = 2.0f;

    // Ursprungs-Pad je Finger -> dessen LIVE-Position (für den Helligkeits-
    // Fade, User 06.07.2026: je weiter die Sonne vom Antipp-Pad wegwandert,
    // desto mehr fällt die Aufhellung zurück; nähert sie sich wieder, hellt
    // das Pad wieder auf).
    std::map<int, juce::Point<float>> activePads;
    for (const auto& entry : fingers)
        activePads[entry.second.padIndex] = entry.second.currentPos;

    const auto fadeDistance = juce::jmax (padWidth, padHeight);

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const auto padIndex = row * cols + col;
            const auto padBounds = juce::Rectangle<float> (padWidth * (float) col, padHeight * (float) row,
                                                            padWidth, padHeight)
                                        .reduced (gap * 0.5f);

            const auto activeIt = activePads.find (padIndex);
            const auto isActive = activeIt != activePads.end();

            if (isActive)
            {
                const auto padCentre = padBounds.getCentre();
                const auto distance  = activeIt->second.getDistanceFrom (padCentre);
                const auto closeness = juce::jlimit (0.0f, 1.0f, 1.0f - distance / fadeDistance);

                g.setColour (push::colours::tile.interpolatedWith (push::colours::tileActive, closeness));
            }
            else
            {
                g.setColour (push::colours::tile);
            }

            g.fillRoundedRectangle (padBounds, 4.0f);
        }
    }

    const auto sunDiameter  = ring.restRadiusPx() * 2.0f;
    const auto moonDiameter = sunDiameter * 0.6f; // 60% der Sonnengröße (User 06.07.2026)

    for (const auto& circle : ring.activeCircles())
    {
        g.setColour (push::colours::ledWhite);

        // "Sonne": ausgemalter Punkt am (ggf. mitwandernden) Zentrum des
        // primären Fingers — fixer Zielpunkt für Pitch/Press unabhängig vom
        // Orbit-Radius (User 06.07.2026, wichtig sobald Hold dazukommt).
        g.fillEllipse (juce::Rectangle<float> (sunDiameter, sunDiameter).withCentre (circle.center));

        if (circle.hasOrbit)
        {
            // Umlaufbahn: dünner Ring durch die aktuelle (ggf. eingefrorene)
            // Ring-Distanz, "Planet" (Mond) an der Ring-Position.
            const auto orbitDiameter = circle.radiusPx * 2.0f;
            g.drawEllipse (juce::Rectangle<float> (orbitDiameter, orbitDiameter).withCentre (circle.center), 1.5f);
            g.fillEllipse (juce::Rectangle<float> (moonDiameter, moonDiameter).withCentre (circle.orbitPos));
        }
    }
}

} // namespace conduit
