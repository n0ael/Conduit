#pragma once

#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/GraphManager.h"
#include "Modules/LinkAudioReceiveModule.h"

namespace conduit
{

//==============================================================================
/**
    Bedien-Panel eines LinkAudioReceiveModule — lebt im NodeComponent
    (Muster LinkAudioSendPanel).

    Drei Zeilen: Kanal-Button (async PopupMenu aus den Session-Kanälen —
    beim Öffnen frisch geholt, kein Listener nötig; „Trennen" löst die
    Bindung), Latenz-Slider (schreibt paramValue von latency_ms — der
    GraphManager spiegelt generisch ins Modul-Atomic, 6.1) und eine
    Status-Zeile (LED offline/searching/waiting/streaming + gepufferte
    Millisekunden als Latenz-Tuning-Hilfe).

    Bindung an den ValueTree-Subtree (5.3): Kanal-Wahl schreibt
    targetPeer/targetChannel, der GraphManager rebindet das Modul.
    Externe Änderungen (Undo, Preset-Load, OSC) kommen über den
    ValueTree-Listener zurück. Status via Timer mit transienter
    Modul-Auflösung pro Tick (kein Processor-Pointer, Zombie-UI-Schutz).
*/
class LinkAudioReceivePanel final : public juce::Component,
                                    private juce::ValueTree::Listener,
                                    private juce::Timer
{
public:
    LinkAudioReceivePanel (juce::ValueTree nodeTreeToBind, GraphManager& graphManagerToUse);
    ~LinkAudioReceivePanel() override;

    /** Teardown-Hook (Phase 1, 5.3): Updates + Interaktion sofort stoppen. */
    void stopUpdates();

    /** Status + Kanal-Text jetzt aus Tree/Modul ziehen — vom Timer gerufen,
        public für headless Tests. */
    void refreshNow();

    /** Höhe der Kachel-Innenfläche (NodeComponent-Sizing). */
    [[nodiscard]] static int panelHeight() noexcept
    {
        return topPadding + 3 * rowHeight + bottomPadding;
    }

    /** Kanal-Wahl anwenden (auch Test-Zugang): schreibt targetPeer/
        targetChannel in den Subtree; leere Namen trennen. */
    void applyChannelChoice (const juce::String& peer, const juce::String& channel);

    void resized() override;
    void paint (juce::Graphics& g) override;

    //==========================================================================
    // Controls public — die UI-Tests treiben sie direkt (Muster Send-Panel)
    juce::TextButton channelButton;
    juce::Slider latencySlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

private:
    //==========================================================================
    // juce::ValueTree::Listener [Message Thread]
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    void timerCallback() override { refreshNow(); }

    void showChannelMenu();
    [[nodiscard]] juce::ValueTree latencyParam() const;
    [[nodiscard]] juce::Rectangle<int> statusRowBounds() const;

    static constexpr int rowHeight     = 30;
    static constexpr int topPadding    = 6;
    static constexpr int bottomPadding = 8;

    juce::ValueTree nodeTree;   // NUR der Subtree (5.3)
    GraphManager& graphManager;
    const juce::String nodeUuid;

    bool updatesStopped = false;

    // Anzeige-Zustand der Status-Zeile (vom Timer gefüllt)
    LinkAudioReceiveModule::ReceiveStatus status = LinkAudioReceiveModule::ReceiveStatus::offline;
    float bufferedSeconds = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LinkAudioReceivePanel)
};

} // namespace conduit
