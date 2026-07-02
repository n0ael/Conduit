#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "OscAddress.h"

namespace conduit
{

class GraphManager;
class ModuleFactory;

//==============================================================================
/**
    Bindet Max4Live-Announces an native Module (CLAUDE.md 7.4).

    Ein Device meldet sich mit /conduit/announce (remoteId, factoryKey,
    Track-Name, Track-Farbe) — find-or-create über die remoteId:

      - Node mit dieser remoteId existiert → idempotent: nur der Tint wird
        nachgezogen. Der Name ist nach der Erst-Anlage User-Hoheit, ein
        Re-Announce (Device-Reload, 30-s-Heartbeat) fasst ihn nicht an.
      - Unbekannt → Whitelist-Check (ModuleFactory::isRegistered), dann
        addModuleNode mit configure-Hook (setzt remoteId + tintColour VOR
        dem Einhängen), danach renameNode auf den Track-Namen (saniert
        selbst; Namens-Kollision → Auto-Name bleibt, die Alias-Adresse
        /conduit/remote/{remoteId}/... funktioniert unabhängig davon).

    Garbage (ungültige remoteId, unbekannter factoryKey) wird still
    verworfen — kein Auth am Announce (LAN-Annahme wie der übrige Empfang),
    Whitelist + Zeichen-Limits + Idempotenz decken das ab.

    Nur Message Thread (der OscController marshallt die Announces).
*/
class RemoteModuleBinder final
{
public:
    RemoteModuleBinder (juce::ValueTree rootTree,
                        GraphManager& graphManagerToUse,
                        ModuleFactory& factoryToUse);

    /** find-or-create — der gebundene Node-Subtree, invalid bei verworfenem
        Announce (Whitelist/Validierung/Factory-Fehler). Message Thread. */
    juce::ValueTree handleAnnounce (const osc::AnnounceInfo& info);

private:
    [[nodiscard]] juce::ValueTree findByRemoteId (const juce::String& remoteId) const;

    /** Einfache Kaskade für announce-erzeugte Kacheln — der User räumt um. */
    [[nodiscard]] juce::Point<int> nextPosition() const;

    juce::ValueTree rootState;  // ref-counted Handle
    GraphManager& graphManager;
    ModuleFactory& factory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RemoteModuleBinder)
};

} // namespace conduit
