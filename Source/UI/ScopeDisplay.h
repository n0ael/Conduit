#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/GraphManager.h"
#include "UiFramePacer.h"

namespace conduit
{

//==============================================================================
/**
    Waveform-Anzeige eines ScopeModule — lebt im NodeComponent.

    Zombie-UI-Regel (5.3): hält KEINEN Pointer auf den Processor. Pro
    Frame-Tick (UiFramePacer: nativ, global gedrosselt) wird das Modul
    transient über GraphManager::getModuleFor() aufgelöst; liefert das
    nullptr (Deleting/entfernt), wird einfach nichts mehr gezogen. Die
    Queue-Konsumentenrolle (SPSC!) liegt exklusiv bei dieser einen Component.

    paint() blockiert nicht: gezeichnet wird nur der lokale Verlaufspuffer.
*/
class ScopeDisplay final : public juce::Component
{
public:
    ScopeDisplay (GraphManager& graphManagerToUse, juce::String nodeUuidToShow);

    /** Teardown-Hook (Phase 1, 5.3): Rendering-Updates sofort stoppen. */
    void stopUpdates();

    /** Zieht alle anstehenden Bins in den Verlaufspuffer — vom Frame-Tick
        gerufen; public für headless Tests (kein Message-Loop in CI). */
    void pullPendingSamples();

    void paint (juce::Graphics& g) override;

private:
    GraphManager& graphManager;
    const juce::String nodeUuid;

    // Ringpuffer des sichtbaren Verlaufs (~3 s @ 750 Bins/s)
    static constexpr int historySize = 2250;
    std::vector<float> historyMin, historyMax;
    int writeIndex = 0;
    bool historyFilled = false;

    // Letzter Member: tickt erst nach vollständiger Konstruktion.
    UiFramePacer framePacer { this, [this] { pullPendingSamples(); } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeDisplay)
};

} // namespace conduit
