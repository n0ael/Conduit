#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/Capture/CaptureService.h"
#include "Core/ChannelNames.h"

namespace conduit
{

//==============================================================================
/**
    Einklappbares Capture-Panel unter der Toolbar — Übergangslösung, bis die
    Kanal-Anzeigen mit dem Mixer-Meilenstein in die Channel-Strips wandern.

    Read/listen-only gegenüber der Engine (CLAUDE.md 6): die Controls
    schreiben ausschließlich in CaptureSettings [Message Thread]. Rechts
    eine Zeile pro Input-Kanal — Status-LED, Mini-Pegel mit Noise-Floor-
    Marker (InputMeter-Atomics) und Einzel-Capture-Button (44 px,
    CaptureService::exportChannel) — gefüttert vom EngineEditor-Timer über
    refresh() (kein eigener Timer, Repaint nur bei quantisierter Änderung).
    Die Kanalanzahl folgt dem aktiven Device: prepare() feuert einen
    ChangeBroadcast, refresh() prüft zusätzlich pro Tick (defensiv).

    Virtuelle Kanäle (Capture-Taps): unter den Hardware-Zeilen erscheint
    ein Abschnitt "Taps" mit denselben Zeilen (LED/Pegel/Einzel-Capture) —
    Zeilenname = registrierter Spurname (moduleId). Register/Unregister/
    Rename feuern einen ChangeBroadcast des Service; ein Tap ohne Puffer
    (Erweiterung wartet auf inaktive Kanäle) zeigt seine Zeile mit
    idle-LED und stummem Pegel.

    Kanal-Namen: Hardware-Zeilen zeigen das effektive ChannelNames-Label
    (userLabel → Device-Name → "In N"). Doppelklick oder Long-Press auf den
    Namen öffnet den Inline-TextEditor des Labels (kein Modal-Loop, 13.2);
    leere Eingabe setzt auf den Default zurück. Tap-Zeilen sind hier nicht
    editierbar — ihr Name ist die moduleId (Rename am Node-Titel).

    Resize-Policy-UI (CaptureSettings-Doku): bufferMinutes/preRollSeconds
    laufen über die Settings-Setter; bei aktiver Aufnahme feuert
    onPendingResize → async Ok/Cancel-AlertWindow
    (JUCE_MODAL_LOOPS_PERMITTED=0) → confirm-/cancelPendingResize.

    "Nach Export freigeben" schaltet nur die Nachfrage frei — der
    RAM-Puffer wird NIE ohne Bestätigung geleert (der Dialog selbst kommt
    vom EngineEditor beim Export-Abschluss).

    Touch-first (10): alle Controls 44 px hoch, Maus/Keyboard-Parität über
    native JUCE-Controls.
*/
class CapturePanel : public juce::Component,
                     private juce::ChangeListener
{
public:
    static constexpr int preferredHeight = 128;

    CapturePanel (CaptureService& serviceToUse, ChannelNames& channelNamesToUse);
    ~CapturePanel() override;

    /** [Message Thread, Editor-Timer] Kanal-Zeilen aus den Status-Atomics
        aktualisieren — Repaint nur bei sichtbarer Änderung. */
    void refresh();

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Toast-Ausgabe des Editors (Einzel-Capture-Feedback). */
    std::function<void (const juce::String&)> onToast;

private:
    //==========================================================================
    /** Eine Kanal-Zeile: Status-LED, Mini-Pegel (RMS-Füllung, Peak-Strich)
        mit Noise-Floor-Marker, Einzel-Capture-Button. Reine Anzeige plus
        Button — gefüttert über setDisplayState() (quantisierte Werte als
        Repaint-Schwelle). */
    class ChannelRow : public juce::Component
    {
    public:
        /** captureIndex = Kanal-Index beim Service (Hardware oder
            virtueller Slot); -1 = Tap ohne Puffer (nur Name + idle-LED).
            onRenameToUse leer → Name nicht editierbar (Tap-Zeilen). */
        ChannelRow (int captureIndexToUse, juce::String nameToUse,
                    std::function<void (int, juce::String)> onCaptureToUse,
                    std::function<void (juce::String)> onRenameToUse = {});

        struct DisplayState
        {
            CaptureChannel::State state = CaptureChannel::State::idle;
            int rmsSteps = 0;    // dB-Position [-80..0] auf 64 Stufen
            int peakSteps = 0;
            int floorSteps = 0;
            bool operator== (const DisplayState&) const = default;
        };

        void setDisplayState (const DisplayState& newState);

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        /** Zeilen-Label mit Inline-Editor: Doppelklick (Label-Standard)
            oder Long-Press (Touch, 10) öffnen den TextEditor. */
        class NameLabel : public juce::Label,
                          private juce::Timer
        {
        public:
            using juce::Label::Label;

            void mouseDown (const juce::MouseEvent& event) override;
            void mouseDrag (const juce::MouseEvent& event) override;
            void mouseUp (const juce::MouseEvent& event) override;

        private:
            static constexpr int longPressMs = 500;
            void timerCallback() override;
        };

        int captureIndex;
        juce::String name;
        DisplayState display;
        NameLabel nameLabel;
        juce::TextButton captureButton { "CAP" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelRow)
    };

    /** Soll-Zustand der Zeilenliste — Hardware-Kanäle plus genutzte
        Tap-Slots; Vergleichsbasis für den Rebuild (Repaint-Disziplin). */
    struct RowSpec
    {
        int captureIndex = -1;
        juce::String name;
        bool isTap = false;
        bool operator== (const RowSpec&) const = default;
    };

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    [[nodiscard]] std::vector<RowSpec> makeRowSpecs() const;
    void rebuildChannelRows();
    void layoutChannelRows();
    void captureSingleChannel (int captureIndex, const juce::String& rowName);

    CaptureService& service;
    ChannelNames& channelNames;

    // Kanal-Zeilen im Viewport (Kanalzahl kann die Panel-Höhe übersteigen)
    juce::Viewport channelViewport;
    juce::Component channelContainer;
    std::vector<std::unique_ptr<ChannelRow>> channelRows;
    std::vector<RowSpec> currentRowSpecs;     // parallel zu channelRows
    juce::Label tapsHeaderLabel { {}, "Taps" };
    juce::Rectangle<int> channelArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CapturePanel)
};

} // namespace conduit
