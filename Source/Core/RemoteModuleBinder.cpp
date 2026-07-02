#include "RemoteModuleBinder.h"

#include "GraphManager.h"
#include "Modules/ConduitModule.h"
#include "Modules/ModuleFactory.h"

namespace conduit
{

RemoteModuleBinder::RemoteModuleBinder (juce::ValueTree rootTree,
                                        GraphManager& graphManagerToUse,
                                        ModuleFactory& factoryToUse)
    : rootState (std::move (rootTree)),
      graphManager (graphManagerToUse),
      factory (factoryToUse)
{
}

//==============================================================================
juce::ValueTree RemoteModuleBinder::handleAnnounce (const osc::AnnounceInfo& info)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (! osc::isValidRemoteId (info.remoteId)
        || ! factory.isRegistered (info.factoryKey))
        return {};

    if (auto existing = findByRemoteId (info.remoteId); existing.isValid())
    {
        // Idempotent: Re-Announce zieht nur den Tint nach (setProperty ist
        // bei gleichem Wert ein No-op) — Name und Position bleiben User-Hoheit
        existing.setProperty (id::tintColour, info.tintArgb, nullptr);
        return existing;
    }

    auto node = graphManager.addModuleNode (info.factoryKey, nextPosition(),
        [&info] (juce::ValueTree& tree)
        {
            tree.setProperty (id::remoteId,   info.remoteId, nullptr);
            tree.setProperty (id::tintColour, info.tintArgb, nullptr);
        });

    if (! node.isValid())
        return {};

    // Wunsch-Name nur bei der Erst-Anlage; renameNode saniert selbst und
    // lehnt Kollisionen ab — dann bleibt der Auto-Name (Alias deckt Max ab)
    graphManager.renameNode (node.getProperty (id::nodeId).toString(),
                             info.trackName);
    return node;
}

//==============================================================================
juce::ValueTree RemoteModuleBinder::findByRemoteId (const juce::String& remoteId) const
{
    return rootState.getChildWithName (id::nodes)
                    .getChildWithProperty (id::remoteId, remoteId);
}

juce::Point<int> RemoteModuleBinder::nextPosition() const
{
    const auto count = rootState.getChildWithName (id::nodes).getNumChildren();
    return { 80 + 36 * (count % 8), 80 + 36 * (count % 8) };
}

} // namespace conduit
