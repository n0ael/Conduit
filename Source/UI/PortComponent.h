#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace conduit
{

//==============================================================================
/** Identität eines Ports — reicht zusammen mit dem Tree, um ein Kabel
    (Connection, Schema 6.2) zu beschreiben. */
struct PortInfo
{
    juce::String nodeUuid;
    bool isInput = false;
    int channel = 0;
};

//==============================================================================
/**
    Ein Anschlusspunkt am Rand einer Node-Kachel. Leitet Kabel-Gesten
    (Drag von Port zu Port) an den NodeCanvas weiter — selbst zustandslos,
    kennt nur seine PortInfo.

    Die Component ist 24×24px (Hit-Zone), gezeichnet wird ein kleinerer
    Kreis; der Canvas erweitert die Treffer-Toleranz beim Drop zusätzlich
    auf Touch-Maß (CLAUDE.md 10).
*/
class PortComponent final : public juce::Component
{
public:
    explicit PortComponent (PortInfo portInfo);

    static constexpr int hitSize = 24;

    [[nodiscard]] const PortInfo& getInfo() const noexcept { return info; }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    const PortInfo info;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PortComponent)
};

} // namespace conduit
