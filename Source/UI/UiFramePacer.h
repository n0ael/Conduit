#pragma once

#include <atomic>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/**
    UI-Framerate-Regel (User-Entscheidung 14.07.2026, ersetzt die alte
    „Meter/Scope 30 fps"-Regel): Anzeige-Refreshes laufen NATIV mit der
    Monitor-Rate (juce::VBlankAttachment), global gedeckelt über
    UiSettings::uiFpsLimit — Default 120 („Nativ, max 120"; VBlank liefert
    ohnehin höchstens die Monitor-Rate), Drossel-Modi 60 und 30.

    uiframe::setFpsLimit() setzt das globale Limit (EngineEditor als
    UiSettings-ChangeListener + Startwert) — bewusst EIN globaler Wert
    statt UiSettings-Referenzen durch jede Meter-Komponente zu fädeln.

    UiFramePacer = VBlankAttachment + Limit-Gate: der Callback feuert pro
    VBlank höchstens einmal und wird unterhalb des Limits übersprungen.
    dt-basierte PHYSIK-Ticks (Federn, Gravity) bleiben direkte
    VBlankAttachments — sie skalieren über ihr Delta selbst.
    Message Thread.
*/
namespace uiframe
{
    inline std::atomic<int>& limitStore() noexcept
    {
        static std::atomic<int> limit { 120 };
        return limit;
    }

    inline void setFpsLimit (int limitFps) noexcept { limitStore().store (limitFps); }
    [[nodiscard]] inline int getFpsLimit() noexcept { return limitStore().load(); }

    /** Gate-Logik (pure, testbar): true wenn der Frame laufen darf;
        aktualisiert lastRunMs dann auf nowMs. */
    [[nodiscard]] inline bool shouldRunFrame (double nowMs, double& lastRunMs, int limitFps) noexcept
    {
        if (limitFps >= 120)
        {
            lastRunMs = nowMs;
            return true;   // Nativ: jeder VBlank (max 120 praktisch nie erreicht)
        }

        const auto intervalMs = 1000.0 / (double) juce::jmax (1, limitFps);

        // 1-ms-Toleranz gegen VSync-Beat: ein 60-Hz-Limit auf einem
        // 60-Hz-Monitor darf nicht auf 30 fps kippen, weil die Timestamps
        // minimal jittern.
        if (nowMs - lastRunMs < intervalMs - 1.0)
            return false;

        lastRunMs = nowMs;
        return true;
    }
}

//==============================================================================
class UiFramePacer final
{
public:
    /** owner = die zu refreshende Komponente (VBlank haengt an ihrem
        Display), tickToUse laeuft gedrosselt (s. o.). */
    UiFramePacer (juce::Component* owner, std::function<void()> tickToUse)
        : tick (std::move (tickToUse)),
          // ACHTUNG Einheiten-Falle (User-Feldtest 14.07.2026, Freeze bei
          // 60/30): ComponentPeer::onVBlank liefert SEKUNDEN (der
          // VBlankAttachment-Parametername "timestampMs" luegt — Windows
          // teilt durch 1000.0) — vor dem ms-Gate umrechnen.
          vblank (owner, [this] (double timestampSec)
          {
              if (! stopped
                  && uiframe::shouldRunFrame (timestampSec * 1000.0, lastRunMs,
                                              uiframe::getFpsLimit())
                  && tick != nullptr)
                  tick();
          })
    {
    }

    /** Refresh dauerhaft anhalten (Phase-1-stopUpdates, Zombie-Schutz 5.3). */
    void stop() noexcept { stopped = true; }

private:
    std::function<void()> tick;
    double lastRunMs = 0.0;
    bool stopped = false;
    juce::VBlankAttachment vblank;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UiFramePacer)
};

} // namespace conduit
