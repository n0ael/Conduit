#include "PortComponent.h"

#include "UI/NodeCanvas.h"

namespace conduit
{

PortComponent::PortComponent (PortInfo portInfo)
    : info (std::move (portInfo))
{
    setSize (hitSize, hitSize);
}

void PortComponent::paint (juce::Graphics& g)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    constexpr float radius = 7.0f;

    g.setColour (juce::Colour (0xff15171a));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (info.isInput ? juce::Colour (0xff7bc8f6) : juce::Colour (0xfff6a97b));
    g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 2.0f);
}

void PortComponent::mouseDown (const juce::MouseEvent& event)
{
    if (auto* canvas = findParentComponentOfClass<NodeCanvas>())
        canvas->beginCableDrag (info, canvas->getLocalPoint (this, event.getPosition()));
}

void PortComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (auto* canvas = findParentComponentOfClass<NodeCanvas>())
        canvas->updateCableDrag (canvas->getLocalPoint (this, event.getPosition()));
}

void PortComponent::mouseUp (const juce::MouseEvent& event)
{
    if (auto* canvas = findParentComponentOfClass<NodeCanvas>())
        canvas->endCableDrag (canvas->getLocalPoint (this, event.getPosition()));
}

} // namespace conduit
