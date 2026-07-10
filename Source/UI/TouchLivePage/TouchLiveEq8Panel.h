#pragma once

#include <array>
#include <map>

#include "TouchLiveBespokePanel.h"
#include "UI/PushTiles.h"

namespace conduit
{

//==============================================================================
/**
    Bespoke EQ-Eight-UI (M5, docs/TouchLive.md §6b): Frequenzgang-Kurve
    mit acht Touch-Punkten statt Fader-Bänken — Darstellung an Lives
    EQ-Eight-Anzeige kalibriert (Messkampagne 10.07.2026, §10i/§10j).

    Parameter-Zuordnung über die parmeta-NAMEN der A-Kurve
    ("{n} Filter On A" · "{n} Filter Type A" · "{n} Frequency A" ·
    "{n} Gain A" · "{n} Q A"/"{n} Resonance A", n = 1…8) — nie über feste
    Indizes; Filtertyp-Semantik aus den items-Strings. Kein vollständiges
    Mapping → isUsable()==false, die DeviceView bleibt bei der Bank.

    Kurvenmathematik: analoge Prototypen (s-Domain) mit Lives
    Q-Semantik — pro Typ ein kalibriertes Q_eff-Gesetz (Bell/Shelf
    gain-abhängig = Adaptive Q; Cuts/Notch 1:1; 48er-Cuts als skalierte
    Butterworth-8-Kaskade). Kalibriert auf < 0.4 dB gegen Lives Anzeige;
    der "Adaptive Q"-Parameter der Gegenseite schaltet den Gain-Term.

    Interaktion: Drag am Punkt = Frequenz (X, log 10 Hz–22 kHz) + Gain
    (Y, nur Bell/Shelf) — lokal-optimistisch (§5.1); Doppeltipp = Band
    an/aus; ZWEITER Finger = Pinch: Abstand ändert den Q des aktiven
    Bandes (Multi-Touch, Kernpfade beginPinch/pinchTo testbar).
    Wertesemantik (verifiziert): Hz = 10·2200^norm, Q = 0.1·180^norm,
    Gain direkt dB.
*/
class TouchLiveEq8Panel final : public TouchLiveBespokePanel
{
public:
    explicit TouchLiveEq8Panel (TouchLiveClient& clientToUse);

    static constexpr int bandCount = 8;
    static constexpr int footerHeight = 40;
    static constexpr float handleDiameter = 44.0f;      // "Zeigefinger"-Punkt
    static constexpr float selectedHandleDiameter = 54.0f;
    static constexpr float touchRadius = 34.0f;         // Trefferzone

    //==========================================================================
    // TouchLiveBespokePanel
    void setDevice (const juce::String& deviceKey, const juce::var& parmeta) override;
    void setValues (const juce::var& parvals) override;
    [[nodiscard]] bool isUsable() const override { return mappedBandCount > 0; }

    //==========================================================================
    // Testbare Kernpfade (Maus-Handler rufen genau diese)

    [[nodiscard]] int bandAt (juce::Point<float> position) const;

    void selectBand (int band);
    void beginDrag (juce::Point<float> position);
    void dragTo (juce::Point<float> position);
    void endDrag();
    void toggleBandOn (int band);
    void stepFilterType (int delta);

    /** Pinch (zweiter Finger): Abstand zum ersten Finger steuert den Q
        des aktiven Bandes — log-proportional zum Abstandsverhältnis. */
    void beginPinch (float distance);
    void pinchTo (float distance);
    void endPinch();

    //==========================================================================
    [[nodiscard]] int getSelectedBand() const noexcept { return selectedBand; }
    [[nodiscard]] int getMappedBandCount() const noexcept { return mappedBandCount; }
    [[nodiscard]] bool isBandOn (int band) const;
    [[nodiscard]] bool isPinchActive() const noexcept { return pinchActive; }
    [[nodiscard]] double getResonanceNorm (int band) const;
    [[nodiscard]] juce::Point<float> bandPosition (int band) const;   // Plot-Pixel
    [[nodiscard]] int frequencyIndexOf (int band) const;
    [[nodiscard]] int gainIndexOf (int band) const;
    [[nodiscard]] int resonanceIndexOf (int band) const;

    juce::Slider qSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    push::TextTile typePrevTile { "<" };
    push::TextTile typeNextTile { ">" };
    push::TextTile bandOnTile { "ON", push::colours::ledOrange };

    //==========================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseDoubleClick (const juce::MouseEvent& event) override;

private:
    //==========================================================================
    enum class Shape { lowCut12, lowCut48, lowShelf, bell, notch,
                       highShelf, highCut12, highCut48 };

    struct Band
    {
        int onIndex = -1, typeIndex = -1, frequencyIndex = -1,
            gainIndex = -1, resonanceIndex = -1;
        juce::StringArray typeItems;

        bool on = false;
        int typeValue = 3;
        double frequencyNorm = 0.5;        // 0..1 (Wire-Wert)
        double gainDb = 0.0;
        double resonanceNorm = 0.5;        // 0..1 (Wire-Wert)
        double gainMin = -15.0, gainMax = 15.0;

        [[nodiscard]] bool isMapped() const noexcept
        {
            return onIndex >= 0 && typeIndex >= 0 && frequencyIndex >= 0
                && gainIndex >= 0 && resonanceIndex >= 0;
        }
    };

    void sendParameter (int parameterIndex, float value, bool continuous);
    void updateFooterFromSelection();
    void rebuildCurve();
    void setResonanceNorm (Band& band, double newNorm);

    [[nodiscard]] Shape shapeOf (const Band& band) const;
    [[nodiscard]] bool shapeHasGain (Shape shape) const noexcept;
    [[nodiscard]] juce::Rectangle<float> plotArea() const;
    [[nodiscard]] float xForNorm (double norm) const;
    [[nodiscard]] double normForX (float x) const;
    [[nodiscard]] float yForDb (double db) const;
    [[nodiscard]] double dbForY (float y) const;

    /** Summen-Magnitude in dB (analoge Prototypen, Live-kalibriert). */
    [[nodiscard]] double responseDbAt (double hz) const;

    /** Lives effektiver RBJ-Q fürs Display (§10j-Kalibrierung). */
    [[nodiscard]] double effectiveQ (Shape shape, double q, double gainDb) const;

    TouchLiveClient& client;
    juce::String deviceKey;

    std::array<Band, bandCount> bands;
    int mappedBandCount = 0;
    int selectedBand = 0;
    bool adaptiveQ = true;      // Parameter "Adaptive Q" der Gegenseite
    int adaptiveQIndex = -1;

    // Touch-Zustand: erster Finger zieht, zweiter pincht (Q)
    bool dragActive = false;
    bool pinchActive = false;
    int primaryTouchIndex = -1, secondaryTouchIndex = -1;
    juce::Point<float> primaryTouchPosition, secondaryTouchPosition;
    float pinchStartDistance = 0.0f;
    double pinchStartResonance = 0.5;

    juce::Path curve;
    bool curveDirty = true;

    static constexpr double plotDbRange = 15.0;   // ±15 dB wie Lives Anzeige

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchLiveEq8Panel)
};

} // namespace conduit
