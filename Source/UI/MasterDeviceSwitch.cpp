#include "MasterDeviceSwitch.h"

#include "PushLookAndFeel.h"

namespace conduit
{

void MasterDeviceSwitch::setFavourites (const juce::StringArray& newFavourites)
{
    if (newFavourites == favourites)
        return;

    favourites = newFavourites;
    repaint();
}

void MasterDeviceSwitch::setCurrent (const juce::String& name)
{
    if (name == current)
        return;

    current = name;
    repaint();
}

int MasterDeviceSwitch::indexOfCurrent() const noexcept
{
    return juce::jmax (0, favourites.indexOf (current));
}

void MasterDeviceSwitch::showIndex (int index)
{
    if (favourites.isEmpty())
        return;

    const auto count = favourites.size();
    const auto wrapped = ((index % count) + count) % count;
    setCurrent (favourites[wrapped]);
}

//==============================================================================
void MasterDeviceSwitch::beginGesture()
{
    gestureActive = true;
    gestureStartIndex = indexOfCurrent();
}

void MasterDeviceSwitch::dragGesture (int totalDeltaY)
{
    if (! gestureActive || favourites.size() < 2)
        return;

    // hoch = vorwärts durch die Liste (ein Schritt je 44 px)
    showIndex (gestureStartIndex + (-totalDeltaY) / kPixelsPerStep);
}

void MasterDeviceSwitch::endGesture (bool wasTap)
{
    if (! gestureActive)
        return;
    gestureActive = false;

    if (wasTap)
        showIndex (gestureStartIndex + 1);   // Tap = nächster Favorit

    if (onMasterChosen != nullptr && current.isNotEmpty())
        onMasterChosen (current);
}

//==============================================================================
void MasterDeviceSwitch::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (gestureActive ? push::colours::tileActive : push::colours::tile);
    g.fillRoundedRectangle (bounds, 4.0f);

    auto area = getLocalBounds().reduced (6, 2);

    g.setColour (push::colours::textDim);
    g.setFont (push::scaledFont (9.0f));
    g.drawText ("MASTER", area.removeFromTop (11), juce::Justification::centredLeft);

    g.setColour (favourites.size() > 1 ? push::colours::text
                                       : push::colours::textDim);
    g.setFont (push::scaledFont (12.0f));
    g.drawText (current.isNotEmpty() ? current : juce::String ("--"),
                area, juce::Justification::centredLeft);
}

void MasterDeviceSwitch::mouseDown (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    beginGesture();
    repaint();
}

void MasterDeviceSwitch::mouseDrag (const juce::MouseEvent& event)
{
    dragGesture (event.getDistanceFromDragStartY());
}

void MasterDeviceSwitch::mouseUp (const juce::MouseEvent& event)
{
    endGesture (event.getDistanceFromDragStart() < kTapTolerancePx
                && getLocalBounds().contains (event.getPosition()));
    repaint();
}

} // namespace conduit
