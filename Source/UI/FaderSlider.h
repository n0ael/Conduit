#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "DragCursorHider.h"

namespace conduit
{

//==============================================================================
/**
    Conduit-Fader-Standard (User-Wunsch 22.07.2026): ein `juce::Slider`, dessen
    Maus-Bedienung sich absichtlich vom JUCE-Default unterscheidet.

    - **Linksklick + Ziehen = RELATIV** (kein Sprung): der Griff bleibt beim
      Zupacken stehen und folgt dem Wischweg (`setSliderSnapsToMousePosition
      (false)`). So verstellt man Werte kontrolliert, ohne dass der Fader unter
      den Cursor springt.
    - **Mittlere Taste (Mausrad-Klick) = SPRINGT** zur Klickposition (das alte
      Snap-to-Mouse-Verhalten) — der schnelle Weg, einen Wert grob zu setzen.
    - **Cursor weg beim Ziehen** (DragCursorHider): relativ beim Links-Drag
      (Cursor kehrt an die Zupack-Stelle zurück), NoCursor beim Mittel-Drag
      (der Griff sitzt ohnehin unter dem Zeiger). Ein reiner Klick blendet
      NICHT aus (Gate auf echte Bewegung); Touch/kein Fenster = No-op.

    Doppelklick-auf-Default, Wertebereich, Response-Kurve (CurvedSlider) und
    alles andere bleiben unberührt — nur die Zeiger-Mechanik ändert sich.
*/
class FaderSlider : public juce::Slider
{
public:
    FaderSlider() { initFaderBehaviour(); }

    FaderSlider (juce::Slider::SliderStyle style,
                 juce::Slider::TextEntryBoxPosition textBoxPosition)
        : juce::Slider (style, textBoxPosition) { initFaderBehaviour(); }

    ~FaderSlider() override { cursorHider.end(); }

    void mouseDown (const juce::MouseEvent& event) override
    {
        // Mittlere Taste springt (Snap für diese eine Geste), links bleibt
        // relativ. Nur bei LINEAREN Fadern — Drehregler springen nie (JUCE
        // ignoriert snapsToMousePos bei Rotary ohnehin).
        middleJump = event.mods.isMiddleButtonDown() && isLinearStyle();
        setSliderSnapsToMousePosition (middleJump);
        juce::Slider::mouseDown (event);
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        if (event.mouseWasDraggedSinceMouseDown())
            cursorHider.begin (*this, event,
                               middleJump ? ui::DragCursorHider::Mode::absolute
                                          : ui::DragCursorHider::Mode::relative);

        juce::Slider::mouseDrag (event);
    }

    void mouseUp (const juce::MouseEvent& event) override
    {
        juce::Slider::mouseUp (event);
        cursorHider.end();
        setSliderSnapsToMousePosition (false);   // zurück auf den relativen Standard
    }

    void resized() override
    {
        juce::Slider::resized();

        // Regelweg = Fader-LÄNGE (User 22.07.2026): der volle Wertebereich
        // braucht so viele Drag-Pixel, wie der Fader lang ist. Dadurch ist ein
        // längerer Fader GENAU um seinen Längenfaktor feiner als ein kürzerer
        // — statt JUCEs fester 250-px-Distanz, die alle gleich empfindlich macht.
        // Nur für LINEARE Fader; Drehregler behalten den JUCE-Default.
        const auto style = getSliderStyle();
        if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical)
            setMouseDragSensitivity (juce::jmax (1, getHeight()));
        else if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
            setMouseDragSensitivity (juce::jmax (1, getWidth()));
    }

private:
    [[nodiscard]] bool isLinearStyle() const
    {
        const auto style = getSliderStyle();
        return style == juce::Slider::LinearHorizontal
            || style == juce::Slider::LinearVertical
            || style == juce::Slider::LinearBar
            || style == juce::Slider::LinearBarVertical;
    }

    void initFaderBehaviour() { setSliderSnapsToMousePosition (false); }

    bool middleJump = false;
    ui::DragCursorHider cursorHider;
};

} // namespace conduit
