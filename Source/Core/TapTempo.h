#pragma once

#include <algorithm>
#include <vector>

#include <juce_core/juce_core.h>

namespace conduit
{

//==============================================================================
/**
    Tap-Tempo als Monitor (inspiriert vom M4L-Device "TAP and CHANGE Tempo
    BPM", User-Entwurf 2026-07-02): Taps messen das Tempo NUR — der Commit
    zur Link-Session passiert außerhalb (Set-Kachel bzw. optionaler
    Auto-Commit fürs MIDI/OSC-Mapping des Tap-Buttons).

    Endloses Tappen: Pausen resetten die Messung NICHT. Intervalle über
    maxIntervalSeconds (< 20 BPM, unplausibel = Pause) gehen nicht in den
    Median ein; der Tap wird neuer Anker, die Messung läuft weiter. Ein
    Reset kommt nur explizit (UI: Tap gedrückt halten).

    Tempo-Schätzung über den MEDIAN eines rollierenden Fensters der letzten
    Intervalle (robust gegen einen verrissenen Tap, folgt Tempowechseln
    beim Dauer-Tappen). Die Zeitbasis liefert der Aufrufer (Sekunden,
    monoton) — dadurch deterministisch testbar. Message Thread.
*/
class TapTempo
{
public:
    /** Intervalle darüber gelten als Pause und werden verworfen. */
    static constexpr double maxIntervalSeconds = 3.0;

    struct Result
    {
        double previewBpm = 0.0;   // gültig wenn hasPreview
        bool   hasPreview = false; // ab dem ersten gültigen Intervall
        bool   committed  = false; // nur bei aktivem Auto-Commit ab Tap n
    };

    /** Auto-Commit (default aus): ab Tap taps committet jeder weitere Tap
        das verfeinerte Tempo — für gemappte Tap-Buttons ohne Set-Klick. */
    void setAutoCommit (bool enabled, int taps) noexcept
    {
        autoCommitEnabled = enabled;
        autoCommitTaps    = juce::jlimit (2, 8, taps);
    }

    Result tap (double timeSeconds)
    {
        if (lastTapTime >= 0.0)
        {
            const auto interval = timeSeconds - lastTapTime;

            // Pause: Intervall verwerfen, Messung behalten (endloses Tappen)
            if (interval > 0.0 && interval <= maxIntervalSeconds)
            {
                intervals.push_back (interval);

                if (intervals.size() > maxIntervals)
                    intervals.erase (intervals.begin());
            }
        }

        lastTapTime = timeSeconds;
        ++tapCount;

        Result result;
        result.hasPreview = hasPreview();
        result.previewBpm = getPreviewBpm();
        result.committed  = autoCommitEnabled && result.hasPreview
                            && tapCount >= autoCommitTaps;
        return result;
    }

    [[nodiscard]] bool hasPreview() const noexcept { return ! intervals.empty(); }

    [[nodiscard]] double getPreviewBpm() const
    {
        if (intervals.empty())
            return 0.0;

        const auto median = medianInterval();
        return median > 0.0 ? 60.0 / median : 0.0;
    }

    void reset() noexcept
    {
        intervals.clear();
        lastTapTime = -1.0;
        tapCount = 0;
    }

private:
    [[nodiscard]] double medianInterval() const
    {
        auto sorted = intervals;
        std::sort (sorted.begin(), sorted.end());

        const auto mid = sorted.size() / 2;
        return sorted.size() % 2 == 1
                   ? sorted[mid]
                   : (sorted[mid - 1] + sorted[mid]) * 0.5;
    }

    /** Rollierendes Fenster — klein genug, um Tempowechseln zu folgen. */
    static constexpr std::size_t maxIntervals = 8;

    std::vector<double> intervals;
    double lastTapTime = -1.0;
    int tapCount = 0;
    bool autoCommitEnabled = false;
    int autoCommitTaps = 4;
};

} // namespace conduit
