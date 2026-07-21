#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include "DragCursorHider.h"
#include "PushLookAndFeel.h"

namespace conduit
{

//==============================================================================
/**
    Wiederverwendbares Zahlenfeld im Push-Stil: Wert sichtbar als Text;
    Berührung zeigt eckige Klammern in Akzentfarbe um den Wert; vertikaler
    Swipe ändert den Wert (nach oben = mehr); Doppeltipp setzt den
    Default-Wert. Domänen-agnostisch (kein Wissen über Grid/MPE o. ä.) --
    Stilvorbild LockToggle (selbstmalend, push::colours, skalierende
    Rahmenstärke).

    Component ist eine feste kRowHeight-px hohe Touch-Zone (CLAUDE.md 10).
*/
class NumberFieldBracket final : public juce::Component,
                                 public juce::SettableTooltipClient
{
public:
    struct Config
    {
        double minValue     = 0.0;
        double maxValue     = 100.0;
        double defaultValue = 50.0;
        double step         = 1.0;   // 0 = stufenlos
        int    decimals     = 0;
        double unitsPerPixel = 0.5;
        juce::String label;          // Kürzel links, textDim -- leer = kein Label
    };

    // Clang lehnt eine verschachtelte Config mit In-Class-Defaults als
    // Default-Argument ab ("needed within definition of enclosing class
    // ... outside of member functions") -- Default-Ctor delegiert
    // stattdessen in der .cpp, wo NumberFieldBracket schon vollständig ist
    // (Muster PadGridLayout/RingTouchModel).
    NumberFieldBracket();
    explicit NumberFieldBracket (const Config& cfg);
    ~NumberFieldBracket() override { cursorHider.end(); }

    void setValue (double newValue, juce::NotificationType notification = juce::sendNotification);
    [[nodiscard]] double getValue() const noexcept { return value; }

    /** Füllfarbe der Klammern im berührten Zustand -- Default cyan. */
    void setAccentColour (juce::Colour newColour) noexcept;

    /** Live während des Swipes bzw. bei Doppeltipp-Reset. */
    std::function<void (double)> onValueChanged;
    /** Am Ende der Interaktion (mouseUp / Doppeltipp) -- Persistenz-Hook. */
    std::function<void (double)> onValueCommitted;

    /** Pure Mapping-Funktionen, headless testbar (kein Component nötig). */
    [[nodiscard]] static double clampAndStep (double raw, const Config& cfg) noexcept;
    [[nodiscard]] static double valueFromDrag (double valueAtDragStart, float dragDeltaYPx,
                                               const Config& cfg) noexcept;

    static constexpr int kRowHeight = 44;   // Touch-Zone (CLAUDE.md 10)
    // Rahmenstärke der Klammern = jmax(2px, Höhe * kBorderFactor).
    static constexpr float kBorderFactor = 2.0f / (float) kRowHeight;

private:
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    Config config;
    double value = 0.0;
    double valueAtDragStart = 0.0;
    bool   touched = false;
    juce::Colour accentColour = push::colours::ledCyan;

    ui::DragCursorHider cursorHider;   // Cursor weg beim vertikalen Swipe

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NumberFieldBracket)
};

} // namespace conduit
