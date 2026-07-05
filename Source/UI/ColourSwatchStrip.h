#pragma once

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Wiederverwendbare Farb-Swatch-Leiste (Push-3-Palette + „keine") — geteilt
    von ChannelAttributePanel (Input-Kanal, M-A) und NodeAttributePanel
    (Factory-Node, M-B). Klick meldet die gewählte Farbe (0x00RRGGBB, 0 =
    keine) via onColourChosen; die Auswahl-Markierung folgt setSelected.

    Bewusst KEIN juce::ColourSelector (zu schwer/modal, Touch-first) — eine
    feste kleine Palette genügt und passt zum Push-Look.
*/
class ColourSwatchStrip final : public juce::Component
{
public:
    ColourSwatchStrip();

    /** 0 = keine. Wird ausgelöst bei Klick auf ein Swatch. */
    std::function<void (juce::uint32)> onColourChosen;

    /** Markiert das passende Swatch (kein Callback). */
    void setSelected (juce::uint32 colour);
    [[nodiscard]] juce::uint32 getSelected() const noexcept { return selected; }

    [[nodiscard]] static int preferredWidth() noexcept;
    [[nodiscard]] static int preferredHeight() noexcept;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    struct Swatch
    {
        juce::uint32 colour = 0;  // 0x00RRGGBB, 0 = keine
        juce::Rectangle<int> bounds;
    };

    juce::uint32 selected = 0;
    std::vector<Swatch> swatches;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourSwatchStrip)
};

} // namespace conduit
