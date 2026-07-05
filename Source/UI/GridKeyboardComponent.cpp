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

    fingers[fingerId] = { pos.x, pad };

    engine.noteOn (static_cast<uint32_t> (fingerId), layout.noteForPad (pad), 100);
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
    repaint();
}

void GridKeyboardComponent::mouseUp (const juce::MouseEvent& event)
{
    const auto fingerId = fingerIdFor (event);
    const auto upResult = ring.onUp (static_cast<uint32_t> (fingerId));

    if (upResult.wasRing)
    {
        engine.setSlide (upResult.ringOwner, 0.0f);
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

    std::map<int, bool> activePads;
    for (const auto& entry : fingers)
        activePads[entry.second.padIndex] = true;

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const auto padIndex = row * cols + col;
            const auto padBounds = juce::Rectangle<float> (padWidth * (float) col, padHeight * (float) row,
                                                            padWidth, padHeight)
                                        .reduced (gap * 0.5f);

            const auto isActive = activePads.count (padIndex) > 0;
            g.setColour (isActive ? push::colours::tileActive : push::colours::tile);
            g.fillRoundedRectangle (padBounds, 4.0f);

            g.setColour (isActive ? push::colours::ledCyan : push::colours::outline);
            g.drawRoundedRectangle (padBounds, 4.0f, isActive ? 2.0f : 1.0f);
        }
    }

    g.setColour (push::colours::ledCyan);
    for (const auto& circle : ring.activeCircles())
    {
        const auto diameter = circle.radiusPx * 2.0f;
        g.drawEllipse (juce::Rectangle<float> (diameter, diameter).withCentre (circle.center), 1.5f);
    }
}

} // namespace conduit
