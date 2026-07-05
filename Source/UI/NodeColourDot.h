#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Kleiner leuchtender Farbpunkt in der Node-Kopfzeile (CLAUDE.md 10, M-B).
    Standardfarbe grün; trägt eine Node-Farbe, sobald gesetzt (Quellfarbe der
    ausgehenden Kabel).

    Gesten:
      - Long-Press (~0,5 s ruhig) → onLongPress (öffnet Rename + Farbwahl).
        Auswahl NUR bei Long-Press — kurzer Tap öffnet nichts (User 05.07.).
      - kurzer Tap → onTap (RESERVIERT fürs spätere Modul-Bypass, aktuell
        typ. no-op).

    Als eigenes Child fängt der Punkt die Maus ab — Drücken verschiebt den
    Node NICHT (der übrige Kachel-Griff bleibt fürs Verschieben).
*/
class NodeColourDot final : public juce::Component,
                            public juce::SettableTooltipClient,
                            private juce::Timer
{
public:
    NodeColourDot();

    std::function<void()> onLongPress;
    std::function<void()> onTap;   // reserviert (Bypass, später)

    /** Punktfarbe setzen (repaint). */
    void setDotColour (juce::Colour newColour);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    static constexpr int longPressMs = 500;
    static constexpr int moveThreshold = 8;

    juce::Colour dotColour { 0xff3ddc84 };  // Push ledGreen (Default)
    bool pressing = false;
    bool longPressed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeColourDot)
};

} // namespace conduit
