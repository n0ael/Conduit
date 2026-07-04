#pragma once

#include <juce_core/juce_core.h>

namespace conduit::browser_drag
{

//==============================================================================
/** Drag-and-drop-Payload zwischen BrowserListRow (Quelle) und NodeCanvas
    (Ziel): "conduit.module:{factoryKey}" — EINE Definition für beide
    Seiten, damit Quelle und Ziel nie auseinanderlaufen. */

inline constexpr const char* modulePrefix = "conduit.module:";

[[nodiscard]] inline juce::String makeModulePayload (const juce::String& factoryKey)
{
    return modulePrefix + factoryKey;
}

/** Leer, wenn die Description kein Modul-Payload ist. */
[[nodiscard]] inline juce::String extractFactoryKey (const juce::String& description)
{
    if (! description.startsWith (modulePrefix))
        return {};

    return description.fromFirstOccurrenceOf (modulePrefix, false, false);
}

} // namespace conduit::browser_drag
