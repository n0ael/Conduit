#include "NodeUiRegistry.h"

namespace conduit
{

void NodeUiRegistry::acquire (const juce::String& nodeUuid)
{
    JUCE_ASSERT_MESSAGE_THREAD
    ++refCounts[nodeUuid];
}

void NodeUiRegistry::release (const juce::String& nodeUuid)
{
    JUCE_ASSERT_MESSAGE_THREAD

    const auto it = refCounts.find (nodeUuid);
    jassert (it != refCounts.end());  // release() ohne passendes acquire()

    if (it == refCounts.end())
        return;

    if (--(it->second) > 0)
        return;

    refCounts.erase (it);

    if (onNodeFullyReleased != nullptr)
        onNodeFullyReleased (nodeUuid);
}

int NodeUiRegistry::getRefCount (const juce::String& nodeUuid) const
{
    const auto it = refCounts.find (nodeUuid);
    return it != refCounts.end() ? it->second : 0;
}

void NodeUiRegistry::setOnNodeFullyReleased (std::function<void (const juce::String&)> callback)
{
    onNodeFullyReleased = std::move (callback);
}

} // namespace conduit
