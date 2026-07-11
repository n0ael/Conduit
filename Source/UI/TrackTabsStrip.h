#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "TouchLive/LiveSetModel.h"
#include "TrackSelectorPanel.h"

namespace conduit
{

//==============================================================================
/**
    Track-Tabs der Grid-Page (Block H3, User-Wunsch 11.07.2026 abends):
    alle Ableton-MIDI-Tracks als Tabs in der obersten Zeile — Name mit
    dünner Umrandung in der Live-Track-Farbe, der Conduit-Fokus-Track
    gefüllt. Tap auf einen Tab wechselt den Fokus (onTrackChosen, gleicher
    Command-Weg wie der TrackSelectorPanel) — „spielerischer" Wechsel ohne
    Long-Press-Menü; der Selector bleibt für die Übersicht bestehen.

    Datenquelle ist die tracks-Domain des LiveSetModel; der Besitzer ruft
    refresh() bei Domain-Änderungen (GridPage::refreshTrackFocus).
    Stable-IDs sind Laufzeit-IDs — NIE serialisieren. Message Thread.
*/
class TrackTabsStrip final : public juce::Component
{
public:
    explicit TrackTabsStrip (LiveSetModel& modelToUse);

    /** Tab angetippt (Stable-ID) — Besitzer sendet das Fokus-Command. */
    std::function<void (const juce::String& stableKey)> onTrackChosen;

    /** Rows + Fokus aus der tracks-Domain neu lesen (repaint bei Delta). */
    void refresh();

    [[nodiscard]] int tabCount() const noexcept { return (int) rows.size(); }

    /** [Tests/Maus] Tab-Index an einer X-Position, -1 = keiner. */
    [[nodiscard]] int tabIndexAt (int x) const noexcept;

    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent& event) override;

    static constexpr int kMaxTabWidth = 220;

private:
    [[nodiscard]] int tabWidth() const noexcept;

    LiveSetModel& model;
    std::vector<TrackSelectorPanel::TrackRow> rows;
    juce::String focusKey;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackTabsStrip)
};

} // namespace conduit
