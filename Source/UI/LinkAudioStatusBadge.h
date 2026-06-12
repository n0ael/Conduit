#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/GraphManager.h"
#include "Modules/LinkAudioSendModule.h"

namespace conduit
{

//==============================================================================
/**
    Status-Badge eines LinkAudioSendModule — lebt im NodeComponent.

    „announced" vs. „streaming" (7.2: Sinks senden erst bei Subscriber):
    Erkennung über die commit-Aktivität des Audio-Threads, gespiegelt im
    SendStatus-Atomic des Moduls. Grenze (siehe Modul-Doku): ein Overrun ist
    von „kein Subscriber" nicht unterscheidbar; ohne laufendes Audio ändert
    sich der Status nicht.

    Zombie-UI-Regel (5.3): hält KEINEN Pointer auf den Processor — das Modul
    wird pro Tick transient über GraphManager::getModuleFor() aufgelöst
    (Muster ScopeDisplay). Reines Display, keine Touch-Interaktion;
    paint() liest nur den lokal gecachten Status.
*/
class LinkAudioStatusBadge final : public juce::Component,
                                   private juce::Timer
{
public:
    LinkAudioStatusBadge (GraphManager& graphManagerToUse, juce::String nodeUuidToShow);

    /** Teardown-Hook (Phase 1, 5.3): Updates sofort stoppen. */
    void stopUpdates();

    /** Status jetzt aus dem Modul ziehen — vom Timer gerufen; public für
        headless Tests (kein Message-Loop in CI). */
    void refreshNow();

    void paint (juce::Graphics& g) override;

    [[nodiscard]] LinkAudioSendModule::SendStatus getShownStatus() const noexcept { return shownStatus; }

private:
    void timerCallback() override;

    GraphManager& graphManager;
    const juce::String nodeUuid;

    LinkAudioSendModule::SendStatus shownStatus = LinkAudioSendModule::SendStatus::offline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LinkAudioStatusBadge)
};

} // namespace conduit
