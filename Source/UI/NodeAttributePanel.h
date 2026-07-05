#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "UI/ColourSwatchStrip.h"

namespace conduit
{

//==============================================================================
/**
    Attribut-Panel eines Factory-Nodes (CLAUDE.md 10, M-B) — Inhalt einer
    CallOutBox, geöffnet per Long-Press auf den Header-Farbpunkt. Bündelt
    Umbenennen + Farbwahl an EINER Stelle.

    Entkoppelt vom GraphManager über Callbacks (Muster CurveEditor):
      - onRename → graphManager.renameNode; false = abgelehnt (Name vergeben/
        ungültig), das Feld springt zurück auf den aktuellen Namen.
      - onColour → graphManager.setNodeColour (0 = keine Farbe).

    Die Farbe ist die Quellfarbe der ausgehenden Kabel (M-B, NodeCanvas).
*/
class NodeAttributePanel final : public juce::Component
{
public:
    NodeAttributePanel (const juce::String& currentName, juce::uint32 currentColour);

    std::function<bool (juce::String)> onRename;   // true = akzeptiert
    std::function<void (juce::uint32)> onColour;    // 0 = keine

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::String currentName;

    juce::Label nameCaption;
    juce::Label nameEditor;        // editierbar → onRename
    juce::Label colourCaption;
    ColourSwatchStrip colourStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeAttributePanel)
};

} // namespace conduit
