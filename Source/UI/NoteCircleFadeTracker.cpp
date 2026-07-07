#include "NoteCircleFadeTracker.h"

#include <algorithm>

namespace conduit
{

void NoteCircleFadeTracker::update (const std::vector<grid::GridVoiceEngine::VoiceReadout>& active,
                                    double deltaMs)
{
    for (auto& circle : tracked)
        circle.stillActive = false;

    for (const auto& readout : active)
    {
        const auto it = std::find_if (tracked.begin(), tracked.end(),
            [&] (const Circle& circle) { return circle.voiceIndex == readout.voiceIndex; });

        if (it != tracked.end())
        {
            it->rawValue    = readout.rawValue;
            it->opacity     = 1.0f;
            it->stillActive = true;
        }
        else
        {
            tracked.push_back ({ readout.voiceIndex, readout.rawValue, 1.0f, true });
        }
    }

    const auto fadeStep = fadeMs > 0 ? (float) (deltaMs / (double) fadeMs) : 1.0f;

    for (auto& circle : tracked)
        if (! circle.stillActive)
            circle.opacity = juce::jmax (0.0f, circle.opacity - fadeStep);

    tracked.erase (std::remove_if (tracked.begin(), tracked.end(),
        [] (const Circle& circle) { return ! circle.stillActive && circle.opacity <= 0.0f; }),
        tracked.end());
}

} // namespace conduit
