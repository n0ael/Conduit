#include "NodeColourDot.h"

namespace conduit
{

NodeColourDot::NodeColourDot()
{
    setTooltip (juce::String::fromUTF8 ("Halten: Umbenennen + Farbe"));
}

void NodeColourDot::setDotColour (juce::Colour newColour)
{
    if (dotColour == newColour)
        return;

    dotColour = newColour;
    repaint();
}

void NodeColourDot::paint (juce::Graphics& g)
{
    // Visuell klein (~9px) — die Touch-Area ist die ganze Ecke (Component-Bounds)
    const auto centre = getLocalBounds().toFloat().getCentre();
    constexpr float coreRadius = 4.5f;

    // Weicher Glow-Hof
    g.setColour (dotColour.withAlpha (0.30f));
    g.fillEllipse (juce::Rectangle<float> (coreRadius * 2.6f, coreRadius * 2.6f).withCentre (centre));

    // Kern
    g.setColour (dotColour);
    g.fillEllipse (juce::Rectangle<float> (coreRadius * 2.0f, coreRadius * 2.0f).withCentre (centre));

    // dezenter Innen-Highlight
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawEllipse (juce::Rectangle<float> (coreRadius * 2.0f, coreRadius * 2.0f).withCentre (centre), 1.0f);
}

void NodeColourDot::mouseDown (const juce::MouseEvent&)
{
    pressing = true;
    longPressed = false;
    startTimer (longPressMs);
}

void NodeColourDot::mouseDrag (const juce::MouseEvent& event)
{
    // Wegziehen bricht die Geste ab (kein Panel, kein Tap)
    if (pressing && event.getDistanceFromDragStart() > moveThreshold)
    {
        stopTimer();
        pressing = false;
    }
}

void NodeColourDot::mouseUp (const juce::MouseEvent&)
{
    stopTimer();

    // Loslassen vor Ablauf = kurzer Tap (reserviert fürs Bypass, später)
    if (pressing && ! longPressed && onTap != nullptr)
        onTap();

    pressing = false;
}

void NodeColourDot::timerCallback()
{
    stopTimer();

    if (pressing)
    {
        longPressed = true;  // mouseUp löst dann keinen Tap aus
        if (onLongPress != nullptr)
            onLongPress();
    }
}

} // namespace conduit
