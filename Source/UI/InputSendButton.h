#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/ChannelNames.h"
#include "Core/InputLinkSend.h"

namespace conduit
{

//==============================================================================
/**
    Link-Send-Toggle einer audio_in-Kanal-Zeile (CLAUDE.md 7.2): schaltet den
    eingebetteten Send des Anker-Ports (mono bzw. Stereo-Paar) und zeigt den
    Kanal-Status als LED (offline grau / announced gelb / streaming grün —
    Farben wie LinkAudioSendPanel).

    Zustand lebt in ChannelNames (linkSendEnabled, App-Zustand) — der Klick
    schreibt NUR das Flag; der ChannelNames-Broadcast zieht Engine
    (EngineProcessor::rebuildInputSends, diff-basiert am lebenden Sink) und
    Port-UI (NodeComponent::rebuildPorts baut auch diese Buttons neu) nach.

    Status-Poll mit 10 Hz vom InputLinkSend (atomics, beliebiger Thread) —
    Muster LinkAudioSendPanel, kein Processor-Pointer (5.3); stopUpdates()
    stoppt den Timer im Teardown. sendService darf nullptr sein (Tests) —
    dann bleibt die LED grau.

    Hit-Zone 24px wie Ports/Koppel-Toggles (bewusste Ausnahme vom
    44px-Touch-Ziel, CLAUDE.md 10).
*/
class InputSendButton final : public juce::Component,
                              public juce::SettableTooltipClient,
                              private juce::Timer
{
public:
    InputSendButton (ChannelNames& channelNamesToUse,
                     const InputLinkSend* sendServiceToUse,
                     int anchorPortToUse);
    ~InputSendButton() override;

    /** Teardown Phase 1 (5.3): keine Status-Polls mehr. */
    void stopUpdates();

    [[nodiscard]] int getAnchorPort() const noexcept { return anchorPort; }

    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    ChannelNames& channelNames;
    const InputLinkSend* sendService;  // nullptr in Tests
    const int anchorPort;

    LinkSendTaps::Status status = LinkSendTaps::Status::offline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputSendButton)
};

} // namespace conduit
