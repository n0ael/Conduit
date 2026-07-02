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

//==============================================================================
/** Validierte Nutzlast eines /conduit/announce (7.4) — vom Netzwerk-Thread
    geparst, auf dem Message Thread konsumiert (RemoteModuleBinder). */
struct AnnounceInfo
{
    juce::String remoteId;    // beidseitig persistent (Live-Set + Patch)
    juce::String factoryKey;  // muss in der ModuleFactory registriert sein
    juce::String trackName;   // Wunsch-Name (wird saniert, nur Erst-Anlage)
    int tintArgb = 0;         // Track-Farbe (0x00RRGGBB aus der Live API)
};

/** remoteIds werden Teil von OSC-Adressen — deshalb hartes Zeichen-Limit
    ([A-Za-z0-9_-], max. 64) statt Sanitizing: ein umgeschriebenes remoteId
    fände sein Gegenstück im Live-Set nie wieder. */
[[nodiscard]] inline bool isValidRemoteId (const juce::String& remoteId)
{
    if (remoteId.isEmpty() || remoteId.length() > 64)
        return false;

    return remoteId.containsOnly (
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-");
}

} // namespace conduit::osc
