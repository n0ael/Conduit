#pragma once

#include <juce_osc/juce_osc.h>

#include "Core/MacroBindings.h"
#include "TouchLiveClient.h"

namespace conduit::grid
{

//==============================================================================
/** MacroTarget auf einen Ableton-Live-Device-Parameter (Block E, Masterplan:
    "Ableton-Parameter direkt ueber das vorhandene TouchLive-Remote-Script --
    Parameter-Browser statt MIDI-Learn").

    Adressierung wie TouchLiveDeviceView::sendParameter: Device-Stable-ID
    (dvid) + Parameter-Index; der 0..1-Macro-Wert wird auf den nativen
    [min,max]-Bereich aus parmeta skaliert (quantisierte Parameter auf ganze
    Schritte gerundet). Versand ueber den 16-ms-Fast-Path (sendTouchValue)
    mit Echo-Suppression (noteTouchedParameter).

    ACHTUNG (Rule touchlive): dvid ist eine LAUFZEIT-Stable-ID -- dieses
    Ziel ist Laufzeit-only und wird NIE serialisiert. Die Block-K-Persistenz
    muss ueber stabile Merkmale (Track-Position + Device-Position +
    Parameter-NAME) re-resolven; nach TouchLiveClient::onReconnected sind
    bestehende Ziele potenziell stale (TODO(design): Re-Resolve). */
class AbletonParamTarget final : public MacroTarget
{
public:
    AbletonParamTarget (TouchLiveClient& clientToUse, juce::String deviceIdToUse,
                        int parameterIndexToUse, float minValueToUse, float maxValueToUse,
                        bool quantisedToUse, juce::String displayNameToUse);

    void sendValue (float value01) override;
    [[nodiscard]] juce::String describe() const override;

    /** Reines Wert-Mapping 0..1 → nativer Live-Bereich (headless testbar). */
    [[nodiscard]] static float mapToNative (float value01, float rangeMin, float rangeMax,
                                            bool snapToSteps) noexcept;

private:
    TouchLiveClient& client;
    juce::String deviceId;      // dvid, Laufzeit-Stable-ID (nie serialisieren)
    int   parameterIndex;
    float minValue, maxValue;
    bool  quantised;
    juce::String displayName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AbletonParamTarget)
};

} // namespace conduit::grid
