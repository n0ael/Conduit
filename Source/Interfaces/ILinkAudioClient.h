#pragma once

#include <juce_core/juce_core.h>

namespace conduit
{

class LinkClock;

//==============================================================================
/**
    Mixin-Interface (CLAUDE.md 4.2-Stil): Module, die Link-Audio-Session-
    Ressourcen halten (Sinks, später Sources — 7.2). Alle Methoden laufen auf
    dem Message Thread.

    Der GraphManager injiziert den Kontext bei der Materialisierung
    (5.2 Schritt 1) — VOR prepareForGraph(), denn der Sink entsteht in
    prepareToPlay() und braucht Clock + moduleId (Kanal-Name == moduleId).
*/
class ILinkAudioClient
{
public:
    virtual ~ILinkAudioClient() = default;

    /** Message Thread, vor prepareForGraph(). clock darf nullptr sein
        (Tests, kein Link) — das Modul bleibt dann ohne Session-Kanal. */
    virtual void setLinkAudioContext (LinkClock* clock,
                                      const juce::String& initialModuleId) = 0;

    /** Rename der named_id (renameNode, auch via Undo) — der Kanal-Name wird
        live zu den Peers propagiert (sink.setName(), 7.2). Message Thread. */
    virtual void moduleIdRenamed (const juce::String& newModuleId) = 0;

    /** Phase 1 des zweiphasigen Deletes (5.3, Pattern OscController):
        Session-Kanäle SOFORT zurückziehen — nicht erst im Destruktor,
        sonst Zombie-Kanäle bei den Peers (7.2). Message Thread. */
    virtual void releaseSessionResources() = 0;
};

} // namespace conduit
