#pragma once

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/MacroBindings.h"

namespace conduit
{

//==============================================================================
/**
    MAP-MODE-Overlay der Looper-Page (07/2026, Muster
    CcControlLayer::setMapMode): liegt über dem Editor-Content (Page +
    Dock) und zeichnet für jedes mappbare Element einen cyanen Rahmen +
    CC-Badge. Es kennt die Widgets NICHT — der EngineEditor liefert die
    Ziel-Liste (Key + Editor-relative Bounds + Badge) und baut sie bei
    Struktur-/Layout-Änderungen neu. Ein Tap auf ein Ziel armiert das
    Learn (klassisch: danach Controller bewegen); das Overlay schluckt
    alle Maus-Events unter sich. Message Thread.
*/
class LooperMapOverlay final : public juce::Component
{
public:
    LooperMapOverlay();

    struct TargetSpot
    {
        grid::MacroControlKey key;
        juce::Rectangle<int> bounds;   // Overlay-relative Koordinaten
        juce::String badge;            // "CC 20" / "Note 61" / leer = frei
    };

    void setTargets (std::vector<TargetSpot> newTargets);

    /** Bereich, in dem Klicks NUR über Zielen abgefangen werden — sonst
        gehen sie durch (das Seitenpanel: MAP-MODE-Toggle und Mappings-
        Liste müssen bedienbar bleiben, sonst sperrt sich der Modus
        selbst ein; User-Fund 20.07.2026). */
    void setPassThroughArea (juce::Rectangle<int> area);

    /** Learn-scharfes Ziel orange markieren; hasArmed=false löscht. */
    void setArmedKey (bool hasArmed, const grid::MacroControlKey& key);

    std::function<void (const grid::MacroControlKey&)> onTargetTapped;

    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent& event) override;
    bool hitTest (int x, int y) override;

private:
    std::vector<TargetSpot> targets;
    juce::Rectangle<int> passThroughArea;
    bool armed = false;
    grid::MacroControlKey armedKey;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperMapOverlay)
};

} // namespace conduit
