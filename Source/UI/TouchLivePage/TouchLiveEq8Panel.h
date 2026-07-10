#pragma once

#include <array>

#include "TouchLiveBespokePanel.h"
#include "UI/PushTiles.h"

namespace conduit
{

//==============================================================================
/**
    Bespoke EQ-Eight-UI (M5, docs/TouchLive.md §6b): Frequenzgang-Kurve
    mit acht Touch-Punkten statt Fader-Bänken.

    Parameter-Zuordnung läuft über die parmeta-NAMEN der A-Kurve
    ("{n} Filter On A" · "{n} Filter Type A" · "{n} Frequency A" ·
    "{n} Gain A" · "{n} Resonance A", n = 1…8) — nie über feste Indizes;
    die Filtertyp-Semantik wird aus den items-Strings des Type-Parameters
    gedeutet (Fallback: Live-12-Reihenfolge). Kein vollständiges Mapping →
    isUsable()==false und die DeviceView bleibt bei der Bank.

    Interaktion: Drag am Punkt = Frequenz (X, log 10 Hz–22 kHz) + Gain (Y,
    nur bei Bell/Shelf) — lokal-optimistisch, sendTouchValue + Suppression
    (Feel-Regeln §5.1); Doppeltipp = Band an/aus. Detail-Leiste unten:
    Typ-Stepper ‹ › · Q-Slider · Band-ON.

    Wertesemantik (Kalibrier-Kandidaten Feldtest, Muster LiveFaderScale):
    Frequency/Resonance reisen normalisiert 0..1 — Anzeige-Mapping
    Hz = 10·2200^v bzw. Q = 0.1·180^v; Gain in dB (parmeta min/max).
    Die Summenkurve ist eine RBJ-Biquad-Näherung fürs Display (48er-Cuts
    als vierfach kaskadierte 12er) — sie steuert nichts.
*/
class TouchLiveEq8Panel final : public TouchLiveBespokePanel
{
public:
    explicit TouchLiveEq8Panel (TouchLiveClient& clientToUse);

    static constexpr int bandCount = 8;
    static constexpr int footerHeight = 40;
    static constexpr float touchRadius = 26.0f;   // Punkt-Trefferzone (44-px-Ziel)

    //==========================================================================
    // TouchLiveBespokePanel
    void setDevice (const juce::String& deviceKey, const juce::var& parmeta) override;
    void setValues (const juce::var& parvals) override;
    [[nodiscard]] bool isUsable() const override { return mappedBandCount > 0; }

    //==========================================================================
    // Testbare Kernpfade (Maus-Handler rufen genau diese)

    /** Band-Index am Punkt (−1 = keiner in touchRadius). */
    [[nodiscard]] int bandAt (juce::Point<float> position) const;

    void selectBand (int band);
    void beginDrag (juce::Point<float> position);
    void dragTo (juce::Point<float> position);
    void endDrag();
    void toggleBandOn (int band);
    void stepFilterType (int delta);

    //==========================================================================
    [[nodiscard]] int getSelectedBand() const noexcept { return selectedBand; }
    [[nodiscard]] int getMappedBandCount() const noexcept { return mappedBandCount; }
    [[nodiscard]] bool isBandOn (int band) const;
    [[nodiscard]] juce::Point<float> bandPosition (int band) const;   // Plot-Pixel
    [[nodiscard]] int frequencyIndexOf (int band) const;
    [[nodiscard]] int gainIndexOf (int band) const;

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

        // Anzeige-Zustand (lokal-optimistisch während Drag)
        bool on = false;
        int typeValue = 3;                 // Index in typeItems
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

    [[nodiscard]] Shape shapeOf (const Band& band) const;
    [[nodiscard]] bool shapeHasGain (Shape shape) const noexcept;
    [[nodiscard]] juce::Rectangle<float> plotArea() const;
    [[nodiscard]] float xForNorm (double norm) const;
    [[nodiscard]] double normForX (float x) const;
    [[nodiscard]] float yForDb (double db) const;
    [[nodiscard]] double dbForY (float y) const;

    /** Summen-Magnitude in dB an einer Frequenz (RBJ-Näherung). */
    [[nodiscard]] double responseDbAt (double hz) const;

    TouchLiveClient& client;
    juce::String deviceKey;

    std::array<Band, bandCount> bands;
    int mappedBandCount = 0;
    int selectedBand = 0;
    bool dragActive = false;

    juce::Path curve;
    bool curveDirty = true;

    static constexpr double plotDbRange = 18.0;   // ±18 dB sichtbar

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TouchLiveEq8Panel)
};

} // namespace conduit
