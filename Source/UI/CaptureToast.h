#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Nicht-modaler Toast ("N Spuren → Ordner") — bewusst eine eigene leichte
    Component, KEIN AlertWindow (10 / 13.2). Klick-transparent, legt sich
    über den Canvas und verschwindet von selbst.

    Der interne Timer ist reine Anzeige-Lebensdauer dieser Component
    (Auto-Hide + Fade) und läuft NUR solange der Toast sichtbar ist — das
    ist nicht der Status-Poll-Pfad der Toolbar (der läuft zentral über den
    EngineEditor-Timer).
*/
class CaptureToast : public juce::Component,
                     private juce::Timer
{
public:
    CaptureToast();

    /** [Message Thread] Toast (erneut) anzeigen — ersetzt laufenden Text. */
    void show (const juce::String& message);

private:
    void paint (juce::Graphics& g) override;
    void timerCallback() override;

    static constexpr int tickIntervalMs = 50;
    static constexpr int holdTicks = 70;   // ~3,5 s Standzeit vor dem Fade

    juce::String text;
    int ticksRemaining = 0;
    float fade = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureToast)
};

} // namespace conduit
