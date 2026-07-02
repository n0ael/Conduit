#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "Modules/ConduitModule.h"

namespace conduit::osc
{

//==============================================================================
// Zentraler OSC-Adressbau (CLAUDE.md 7) — eine Quelle für Receive-Registry
// (OscController::rebuildEndpoints) und Send-Pfad (OscSendService), damit
// beide Seiten garantiert dieselben Adressen sprechen.

/** Voll-Dump-Anfrage eines Clients (7.3). */
inline constexpr const char* syncAddress = "/conduit/sync";

/** Announce eines Max4Live-Devices (7.4):
    s:remoteId s:factoryKey s:trackName i:trackColour */
inline constexpr const char* announceAddress = "/conduit/announce";

/** Kanonische Parameter-Adresse eines Tree-Nodes:
    /conduit/{type}/{moduleId}/{paramId} — Schema 7. */
[[nodiscard]] inline juce::String parameterAddress (const juce::ValueTree& nodeTree,
                                                    const juce::String& parameterId)
{
    return "/conduit/"
           + nodeTree.getProperty (id::type).toString().toLowerCase()
           + "/" + nodeTree.getProperty (id::moduleId).toString()
           + "/" + parameterId;
}

/** Receive-Alias für announce-gebundene Nodes (7.4):
    /conduit/remote/{remoteId}/{paramId} — unabhängig von moduleId-Renames
    und Kollisions-Suffixen; der Send-Pfad bleibt kanonisch. */
[[nodiscard]] inline juce::String remoteAliasAddress (const juce::String& remoteId,
                                                      const juce::String& parameterId)
{
    return "/conduit/remote/" + remoteId + "/" + parameterId;
}

} // namespace conduit::osc
