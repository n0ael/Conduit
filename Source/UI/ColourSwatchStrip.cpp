#include "ColourSwatchStrip.h"

#include "UI/PushLookAndFeel.h"

namespace conduit
{

namespace
{
    constexpr int swatchSize = 24;
    constexpr int swatchGap  = 4;

    // Track-Farbpalette im Push-3-Ton (LED-Akzente + Ableton-nahe Töne).
    // Der letzte Eintrag (0) ist „keine Farbe" → Kabel nutzt die Default-Farbe.
    const std::vector<juce::uint32> paletteColours {
        0x00ff453au,  // rot
        0x00ffa726u,  // orange
        0x00e8c30fu,  // gelb
        0x003ddc84u,  // grün
        0x0000bfd8u,  // cyan
        0x004a90e2u,  // blau
        0x00a066d3u,  // violett
        0x00f0f0f0u,  // weiß
        0u            // keine
    };

    [[nodiscard]] juce::Colour opaque (juce::uint32 rgb)
    {
        return juce::Colour (0xff000000u | (rgb & 0x00ffffffu));
    }
}

//==============================================================================
ColourSwatchStrip::ColourSwatchStrip()
{
    for (const auto rgb : paletteColours)
        swatches.push_back ({ rgb, {} });
}

int ColourSwatchStrip::preferredWidth() noexcept
{
    const auto n = (int) paletteColours.size();
    return n * swatchSize + (n - 1) * swatchGap;
}

int ColourSwatchStrip::preferredHeight() noexcept
{
    return swatchSize;
}

void ColourSwatchStrip::setSelected (juce::uint32 colour)
{
    if (selected == colour)
        return;

    selected = colour;
    repaint();
}

void ColourSwatchStrip::resized()
{
    int x = 0;
    for (auto& swatch : swatches)
    {
        swatch.bounds = { x, 0, swatchSize, swatchSize };
        x += swatchSize + swatchGap;
    }
}

void ColourSwatchStrip::paint (juce::Graphics& g)
{
    for (const auto& swatch : swatches)
    {
        const auto area = swatch.bounds.toFloat().reduced (1.0f);

        if (swatch.colour == 0)
        {
            // „Keine" — leerer Umriss mit diagonalem Strich
            g.setColour (push::colours::outline);
            g.drawRoundedRectangle (area, 4.0f, 1.2f);
            g.drawLine (area.getX() + 4.0f, area.getBottom() - 4.0f,
                        area.getRight() - 4.0f, area.getY() + 4.0f, 1.2f);
        }
        else
        {
            g.setColour (opaque (swatch.colour));
            g.fillRoundedRectangle (area, 4.0f);
        }

        if (swatch.colour == selected)
        {
            g.setColour (push::colours::ledWhite);
            g.drawRoundedRectangle (swatch.bounds.toFloat().reduced (0.5f), 5.0f, 2.0f);
        }
    }
}

void ColourSwatchStrip::mouseDown (const juce::MouseEvent& event)
{
    for (const auto& swatch : swatches)
        if (swatch.bounds.contains (event.getPosition()))
        {
            setSelected (swatch.colour);

            if (onColourChosen != nullptr)
                onColourChosen (swatch.colour);

            return;
        }
}

} // namespace conduit
