#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "LooperPanel.h"
#include "PushTiles.h"

namespace conduit
{

//==============================================================================
/**
    Papierkorb-Kachel der Looper-Kopfzeile (Big Out 07/2026): sichtbar,
    solange der LooperTrashCan Einträge hält; zeigt ↺ + Rest-Zeit m:ss.
    Die letzten warnSeconds faden zu Rot; läuft ein Eintrag ab, flackert
    die Kachel kurz und verschwindet (User-Spezifikation 19.07.2026).
    Flacker-Ticks via Timer::callAfterDelay + SafePointer (kein
    Member-Timer — Lektion NodeEditor).
*/
class LooperTrashTile final : public juce::Button
{
public:
    static constexpr double warnSeconds = 30.0;

    LooperTrashTile();

    /** [Editor-Timer, 15 Hz] Restzeit + Bestand — Repaint nur bei
        sichtbarer Änderung (Sekunden-/Farbschritt). */
    void setTrashState (double secondsRemaining, bool hasEntries);

    /** Ablauf-Flackern: kurzes Blinken, danach folgt die Sichtbarkeit
        wieder dem Bestand (setTrashState). */
    void flashEmptied();

    [[nodiscard]] bool isFlashing() const noexcept { return flashTicks > 0; }

private:
    void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override;
    void scheduleFlashTick();

    double remaining = 0.0;
    bool entries = false;
    int shownSeconds = -1;
    int flashTicks = 0;
    bool flashOn = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperTrashTile)
};

//==============================================================================
/**
    Looper-Page — Vollausbau M6, Kopf-Umbau 07/2026 (Mixer-Handoff):
    bis zu 4 Looper-Fenster NEBENEINANDER; die frühere Kopfzeile ist
    KOMPLETT entfallen — Looper-/Track-Anzahl, Output-Paar und MST leben
    im Seitenpanel (LooperDockTabs), der Spectrum-Umschalter pro Looper
    im Panel-Kopf (FFT). Unten: Statuszeile links, Papierkorb-Kachel +
    Stop All rechts (Stop All per Setting ausblendbar — „mappen, dann
    verstecken").

    Wie die TransportBar besitzt die Page NUR UI-Zustand: Aktionen laufen
    über std::function-Hooks bzw. die Hooks der LooperPanels (der
    EngineEditor verdrahtet Modell/Bank/Settings/Taps und pollt den
    Status über seinen Timer — inkl. der gemeinsamen Puls-Phase der
    Target-Zellen).
*/
class LooperPage final : public juce::Component
{
public:
    LooperPage();

    //==========================================================================
    // Struktur [Editor]

    /** Panels neu aufbauen (1–4). Nach dem Aufbau verdrahtet der Editor
        die Panel-Hooks neu (onPanelsChanged feuert am Ende). */
    void setLooperCount (int count);
    [[nodiscard]] int getLooperCount() const noexcept { return (int) panels.size(); }
    [[nodiscard]] LooperPanel& getPanel (int looperIndex);

    /** Nach jedem Panel-Neuaufbau (Editor verdrahtet die Hooks). */
    std::function<void()> onPanelsChanged;

    //==========================================================================
    // Hooks (Struktur-Hooks feuern seit dem Kopf-Umbau aus dem
    // Seitenpanel-LAYOUT über den Editor — die Page hält sie nur)

    std::function<void()> onAddLooper;
    std::function<void()> onRemoveLooper;
    std::function<void()> onRestoreTrash;      // ↺ Papierkorb-Kachel (Big Out)
    std::function<void()> onStop;              // Stop All = alle Tracks

    /** Spektrum-Ansicht des Loopers (Panel-Kopf-FFT-Kachel + Strip). */
    void setSpectrumView (int looperIndex, bool spectrum);
    void setStatus (const juce::String& statusText);

    /** Sichtbarkeit der Stop-All-Kachel (LooperSettings::isShowStopAll). */
    void setShowStopAll (bool show);

    /** [Editor-Timer] Gemeinsame Puls-Phase (Target-Zellen). */
    void setPulsePhase (float phase01);

    /** [Editor-Timer] Papierkorb-Zustand (Kachel-Sichtbarkeit + Countdown). */
    void setTrashState (double secondsRemaining, bool hasEntries);

    /** Ablauf-Flackern der Papierkorb-Kachel (LooperTrashCan::onExpired). */
    void flashTrashEmptied() { trashTile.flashEmptied(); }

    //==========================================================================
    [[nodiscard]] push::TextTile& getStopAllTile() noexcept { return stopAllTile; }
    [[nodiscard]] LooperTrashTile& getTrashTile() noexcept { return trashTile; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    LooperTrashTile trashTile;
    push::TextTile stopAllTile { "Stop All", push::colours::ledRed };
    juce::Label statusLabel;
    bool showStopAll = true;

    std::vector<std::unique_ptr<LooperPanel>> panels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperPage)
};

} // namespace conduit
