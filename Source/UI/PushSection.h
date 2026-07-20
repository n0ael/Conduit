#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "PushLookAndFeel.h"

namespace conduit::push
{

//==============================================================================
/**
    Einklappbare Panel-Sektion (Mixer/Panel 07/2026) — das ▸/▾-Muster der
    LooperPatchOutPanel-Überschriften als wiederverwendbares Widget: eine
    Kopfzeile mit Dreieck + Titel (Tap toggelt), darunter GENAU EINE
    Content-Komponente des Besitzers. Der Besitzer meldet die Wunschhöhe
    des Contents (setContentHeight) und layoutet die Sektion nach
    getPreferredHeight(); Persistenz des Zustands (z. B. Bitmaske in den
    LooperSettings) läuft über onToggled. Message Thread.
*/
class CollapsibleSection final : public juce::Component
{
public:
    static constexpr int headerHeight = 36;   // volle Breite = Touch-Ziel

    CollapsibleSection (const juce::String& titleToUse, juce::Component& contentToUse)
        : title (titleToUse), content (contentToUse)
    {
        setName ("section_" + titleToUse);
        addAndMakeVisible (content);
    }

    void setContentHeight (int height) noexcept { contentHeight = juce::jmax (0, height); }

    [[nodiscard]] int getPreferredHeight() const noexcept
    {
        return headerHeight + (collapsed ? 0 : contentHeight);
    }

    void setCollapsed (bool shouldCollapse, bool notify = true)
    {
        if (collapsed == shouldCollapse)
            return;

        collapsed = shouldCollapse;
        content.setVisible (! collapsed);
        repaint();

        if (notify && onToggled != nullptr)
            onToggled (collapsed);
    }

    [[nodiscard]] bool isCollapsed() const noexcept { return collapsed; }

    /** Feuert bei jedem User-Toggle (nicht bei setCollapsed(…, false)). */
    std::function<void (bool nowCollapsed)> onToggled;

    void paint (juce::Graphics& g) override
    {
        const auto header = getLocalBounds().removeFromTop (headerHeight);
        g.setColour (colours::textDim);
        g.setFont (scaledFont (12.0f, true));
        const auto arrow = juce::String::fromUTF8 (collapsed ? "\xe2\x96\xb8" : "\xe2\x96\xbe");
        g.drawText (arrow + "  " + title.toUpperCase(), header.reduced (8, 0),
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop (headerHeight);
        content.setBounds (bounds);
    }

    void mouseUp (const juce::MouseEvent& event) override
    {
        if (event.getPosition().y <= headerHeight && ! event.mouseWasDraggedSinceMouseDown())
            setCollapsed (! collapsed);
    }

private:
    juce::String title;
    juce::Component& content;
    int contentHeight = 0;
    bool collapsed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CollapsibleSection)
};

} // namespace conduit::push
