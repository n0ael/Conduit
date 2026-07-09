#pragma once

#include <map>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace conduit
{

//==============================================================================
/**
    Transiente Meter-Werte der TouchLive-Remote (docs/TouchLive.md §3):
    Stereo-Pegel pro Stable-ID aus dem Hochraten-Pfad `/remote/meters`
    (M2) — bewusst NICHT im ValueTree (Muster 4.6-Meter: die UI liest pro
    Tick, cached nie; Struktur-Diffs bleiben davon getrennt).

    Der TouchLiveClient schreibt pro empfangenem Datagramm (ein Frame =
    alle Tracks) und erhöht den Frame-Zähler; die UI vergleicht den Zähler
    und überspringt Ticks ohne neue Daten. Werte sind Lives rohe
    output_meter-Norm (0..1) — Anzeige-Mapping macht die UI.

    Reiner Message-Thread-Zustand, kein Lock (der Netzwerk-Thread reicht
    Messages über die Client-Queue herein).
*/
class TouchLiveMeterBus
{
public:
    struct StereoLevel
    {
        float left  = 0.0f;
        float right = 0.0f;
    };

    TouchLiveMeterBus() = default;

    /** Ein Kanalpaar eines Frames übernehmen. [Message Thread] */
    void update (const juce::String& key, float left, float right)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        levels[key] = { juce::jlimit (0.0f, 1.0f, left),
                        juce::jlimit (0.0f, 1.0f, right) };
    }

    /** Nach dem letzten update() eines Datagramms rufen — die UI erkennt
        daran neue Frames. [Message Thread] */
    void noteFrame() noexcept { ++frameCounter; }

    [[nodiscard]] StereoLevel getLevel (const juce::String& key) const
    {
        JUCE_ASSERT_MESSAGE_THREAD
        const auto it = levels.find (key);
        return it != levels.end() ? it->second : StereoLevel {};
    }

    [[nodiscard]] juce::uint32 getFrameCounter() const noexcept { return frameCounter; }

    /** Disable/Reconnect: alte Pegel verwerfen (UI fällt auf Stille). */
    void clear()
    {
        JUCE_ASSERT_MESSAGE_THREAD
        levels.clear();
        ++frameCounter;
    }

private:
    std::map<juce::String, StereoLevel> levels;
    juce::uint32 frameCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchLiveMeterBus)
};

} // namespace conduit
