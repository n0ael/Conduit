#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    Auswahl-Liste der Papierkorb-Kachel (User 19.07.2026): hält der
    Papierkorb MEHRERE Einträge, öffnet die ↺-Kachel dieses CallOut —
    eine Zeile pro Eintrag (neuester zuoberst, Label + Restzeit), Tap
    stellt genau diesen Eintrag wieder her. Ein einzelner Eintrag wird
    weiterhin direkt wiederhergestellt (kein Dialog).

    Kein Modal-Loop (13.2): CallOutBox, jede Zeile schließt den Dialog
    selbst (Muster LooperDeleteConfirmDialog). Controls public für
    headless Tests. Restzeiten sind beim Öffnen eingefroren — der Dialog
    lebt nur einen Tap lang.
*/
class LooperTrashDialog final : public juce::Component
{
public:
    struct Item
    {
        std::uint32_t entryId = 0;
        juce::String label;       // „Clip: Live / wavetable · L1 T2 S3"
        juce::String timeLabel;   // „2:41"
    };

    explicit LooperTrashDialog (std::vector<Item> items);

    /** Zeilen-Tap — der Aufrufer stellt den Eintrag wieder her. */
    std::function<void (std::uint32_t entryId)> onRestore;

    void resized() override;

    // public für Tests
    std::vector<std::unique_ptr<juce::TextButton>> rowButtons;

private:
    juce::Label titleLabel;
    juce::Viewport viewport;          // > 6 Zeilen scrollen
    juce::Component rowContainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperTrashDialog)
};

} // namespace conduit
