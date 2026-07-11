#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Master-Device-Quick-Switch der Grid-Page (Block H3, User-Wunsch
    11.07.2026 abends): Da „All Ins" im Fokus-Modus bewusst nicht mehr
    verwendet wird, braucht der schnelle Wechsel des Master-MIDI-Geräts
    (Push ↔ MIDI-Keyboard …) eine Performance-Geste: Die Kachel oben
    links zeigt das aktuelle Master-Device; **Tap** springt zum nächsten
    Favoriten, **Push-and-Drag** (vertikal, ein Schritt je 44 px) scrollt
    live durch die Favoriten — Loslassen übernimmt (onMasterChosen).

    Die Favoritenliste pflegt der User im Settings-Tab über das „+" neben
    dem MIDI-Master-Dropdown (persistent, GridPanelSettings). Ohne
    Favoriten zeigt die Kachel nur den aktuellen Namen (keine Geste).
    Message Thread.
*/
class MasterDeviceSwitch final : public juce::Component
{
public:
    MasterDeviceSwitch() = default;

    /** Auswahl committet (mouseUp) — Besitzer persistiert + re-routet. */
    std::function<void (const juce::String& name)> onMasterChosen;

    void setFavourites (const juce::StringArray& newFavourites);
    void setCurrent (const juce::String& name);

    [[nodiscard]] juce::String currentName() const noexcept { return current; }

    //==========================================================================
    // Testbare Kernpfade der Maus-Handler
    void beginGesture();
    void dragGesture (int totalDeltaY);
    void endGesture (bool wasTap);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    static constexpr int kPixelsPerStep = 44;
    static constexpr int kTapTolerancePx = 8;

private:
    [[nodiscard]] int indexOfCurrent() const noexcept;
    void showIndex (int index);

    juce::StringArray favourites;
    juce::String current;

    int  gestureStartIndex = 0;
    bool gestureActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterDeviceSwitch)
};

} // namespace conduit
