#pragma once

#include <functional>
#include <memory>

#include <juce_osc/juce_osc.h>

namespace conduit
{

//==============================================================================
/** Transport-Seam des TouchLiveClient — Tests injizieren einen Fake
    (send/receive ohne echtes UDP), die App den juce::OSCSender/OSCReceiver
    (makeUdpRemoteTransport). Muster: IOscSink des OscSendService (7.3).

    Der MessageHandler wird auf dem NETZWERK-Thread gerufen (Realtime-
    Callback des OSCReceiver) — deshalb VOR connect() setzen und im Handler
    nur queuen, nie ValueTree/Modell anfassen. */
class IRemoteTransport
{
public:
    using MessageHandler = std::function<void (const juce::OSCMessage&)>;

    virtual ~IRemoteTransport() = default;

    /** VOR connect() setzen — läuft auf dem Netzwerk-Thread. */
    virtual void setMessageHandler (MessageHandler handler) = 0;

    /** Bindet den Empfang an listenPort und richtet den Sender auf
        host:commandPort. false, wenn Bind/Connect fehlschlägt. */
    virtual bool connect (const juce::String& host, int commandPort, int listenPort) = 0;

    virtual void disconnect() = 0;

    /** Ein Command/Ping an das Remote Script. */
    virtual bool send (const juce::OSCMessage& message) = 0;
};

/** UDP-Implementierung via juce::OSCSender + juce::OSCReceiver (Port 9011,
    Realtime-Callback → Netzwerk-Thread). */
[[nodiscard]] std::unique_ptr<IRemoteTransport> makeUdpRemoteTransport();

} // namespace conduit
