#include "AbletonParamTarget.h"

#include <cmath>

namespace conduit::grid
{

AbletonParamTarget::AbletonParamTarget (TouchLiveClient& clientToUse, juce::String deviceIdToUse,
                                        int parameterIndexToUse, float minValueToUse, float maxValueToUse,
                                        bool quantisedToUse, juce::String displayNameToUse)
    : client (clientToUse), deviceId (std::move (deviceIdToUse)),
      parameterIndex (parameterIndexToUse),
      minValue (minValueToUse), maxValue (maxValueToUse),
      quantised (quantisedToUse), displayName (std::move (displayNameToUse))
{
}

float AbletonParamTarget::mapToNative (float value01, float rangeMin, float rangeMax,
                                       bool snapToSteps) noexcept
{
    const auto clamped = juce::jlimit (0.0f, 1.0f, value01);
    auto native = rangeMin + (rangeMax - rangeMin) * clamped;

    // Quantisierte Parameter (parmeta.quant, Schrittweite 1.0): auf ganze
    // Schritte runden, damit Live keine Zwischenwerte sieht.
    if (snapToSteps)
        native = std::round (native);

    return native;
}

void AbletonParamTarget::sendValue (float value01)
{
    if (deviceId.isEmpty())
        return;

    // Exakt der Pfad von TouchLiveDeviceView::sendParameter: Suppression
    // fuer die heisse parvals-Zeile + Fast-Path-Versand (16-ms-Thinning).
    client.noteTouchedParameter (TouchLiveClient::makeParameterKey (
        "devices", "parvals:" + deviceId));

    juce::OSCMessage message { juce::OSCAddressPattern ("/live/device/set/parameter") };
    message.addString (deviceId);
    message.addInt32 (parameterIndex);
    message.addFloat32 (mapToNative (value01, minValue, maxValue, quantised));
    client.sendTouchValue (message);
}

juce::String AbletonParamTarget::describe() const
{
    return displayName.isNotEmpty() ? displayName : ("Live-Parameter " + juce::String (parameterIndex));
}

} // namespace conduit::grid
