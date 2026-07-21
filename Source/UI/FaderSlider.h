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
        // Mittlere Taste springt (Snap für diese eine Geste), links bleibt relativ.
        middleJump = event.mods.isMiddleButtonDown();
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

private:
    void initFaderBehaviour() { setSliderSnapsToMousePosition (false); }

    bool middleJump = false;
    ui::DragCursorHider cursorHider;
};

} // namespace conduit
