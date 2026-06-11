#pragma once

#include <functional>
#include <map>

#include <juce_events/juce_events.h>

namespace conduit
{

//==============================================================================
/**
    Zählt UI-Referenzen pro Node — Zombie-UI-Schutz (CLAUDE.md 5.3).

    UI-Components rufen acquire() beim Binden an einen Node-Subtree und
    release() beim Entkoppeln. Beim zweiphasigen Delete entkoppelt sich die
    UI in Phase 1 (Reaktion auf nodeState == Deleting, Freigabe nach dem
    letzten Render-Zyklus via VBlankAttachment); der GraphManager startet
    Phase 2 erst, wenn der Refcount des Nodes 0 ist.

    Nur Message Thread.
*/
class NodeUiRegistry final
{
public:
    NodeUiRegistry() = default;

    void acquire (const juce::String& nodeUuid);

    /** Bei Refcount 0 wird der Eintrag entfernt und onNodeFullyReleased
        benachrichtigt — das stößt eine wartende Phase 2 erneut an. */
    void release (const juce::String& nodeUuid);

    [[nodiscard]] int getRefCount (const juce::String& nodeUuid) const;

    /** Einziger Konsument: der GraphManager. */
    void setOnNodeFullyReleased (std::function<void (const juce::String&)> callback);

private:
    std::map<juce::String, int> refCounts;
    std::function<void (const juce::String&)> onNodeFullyReleased;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeUiRegistry)
};

} // namespace conduit
