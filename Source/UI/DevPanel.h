#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Core/GridPanelSettings.h"
#include "Core/UiSettings.h"

namespace conduit
{

//==============================================================================
/**
    Schwebendes Dev-Panel (User-Wunsch 07/2026): kleines always-on-top-Fenster
    für Live-UI-Tweaks — Inhalt ist dieselbe UiSettingsComponent wie der
    „Oberfläche"-Settings-Tab (derselbe UiSettings-Broadcaster hält beide
    synchron), darunter die Grid-Dev-Werte Schwellbreite/Fade-Zeit (Block A4,
    Umzug aus MpeShapingView -- persistieren weiterhin über
    GridPanelSettings, MpeShapingView pollt die Werte live in tick()). Nur
    im Dev Mode erreichbar (Dev-Tile in der TransportBar).

    Ownership: unique_ptr im EngineEditor. closeButtonPressed meldet nur
    onClose — der Editor destruiert ASYNC (callAsync + SafePointer, ein
    Fenster darf sich nicht aus dem eigenen Callback zerstören) und schließt
    das Panel automatisch, wenn der Dev Mode deaktiviert wird.
*/
class DevPanel final : public juce::DocumentWindow
{
public:
    DevPanel (UiSettings& uiSettingsToUse, GridPanelSettings& gridPanelSettingsToUse);

    std::function<void()> onClose;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DevPanel)
};

} // namespace conduit
