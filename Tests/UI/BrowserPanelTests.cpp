#include <catch2/catch_test_macros.hpp>

#include "Core/Browser/BrowserModel.h"
#include "UI/Browser/BrowserPanel.h"

namespace
{
struct PanelRig
{
    PanelRig()
    {
        conduit::registerDefaultModules (factory);
        panel.setSize (conduit::BrowserPanel::dockWidth, 600);
    }

    juce::ScopedJuceInitialiser_GUI juceRuntime;
    conduit::ModuleFactory factory;
    conduit::BrowserContextProvider context;
    conduit::BrowserModel model { factory, context };
    conduit::BrowserPanel panel { model };
};
} // namespace

//==============================================================================
TEST_CASE ("Browser-Panel: Zeilen spiegeln das Modell, 44-px-Raster", "[browser][ui]")
{
    PanelRig rig;

    REQUIRE (conduit::BrowserPanel::rowHeight >= 44);      // Touch-Target-Regel
    REQUIRE (conduit::BrowserPanel::headerHeight >= 44);
    REQUIRE (rig.panel.getListBox().getRowHeight() >= 44);

    // Übersicht (Device-Default): 3 Bereiche
    REQUIRE (rig.panel.getListBox().getListBoxModel()->getNumRows() == 3);

    rig.model.openSection (conduit::BrowserContextProvider::Section::modules);
    REQUIRE (rig.panel.getListBox().getListBoxModel()->getNumRows()
                 == (int) rig.model.rows().size());
}

TEST_CASE ("Browser-Panel: Öffnen/Schließen steuert die Dock-Breite", "[browser][ui]")
{
    PanelRig rig;

    REQUIRE (rig.panel.currentDockWidth() == 0);
    REQUIRE_FALSE (rig.panel.isOpen());

    int layoutCalls = 0;
    rig.panel.onDockWidthChanged = [&layoutCalls] { ++layoutCalls; };

    // Headless: kein Peer → AnimatedValue springt sofort ans Ziel
    rig.panel.setOpen (true);
    REQUIRE (rig.panel.isOpen());
    REQUIRE (rig.panel.currentDockWidth() == conduit::BrowserPanel::dockWidth);
    REQUIRE (rig.panel.isVisible());
    REQUIRE (layoutCalls > 0);

    rig.panel.setOpen (false);
    REQUIRE (rig.panel.currentDockWidth() == 0);
    REQUIRE_FALSE (rig.panel.isVisible());
}

TEST_CASE ("Browser-Panel: Öffnen navigiert zum Startbereich der Page", "[browser][ui]")
{
    PanelRig rig;

    // Device-Page → direkt MODULE, Zurück-Pfeil sichtbar
    rig.panel.setOpen (true);
    REQUIRE (rig.model.breadcrumbText() == "MODULE");
    REQUIRE (rig.panel.getBackTile().isVisible());

    rig.panel.getBackTile().onClick();
    REQUIRE (rig.model.breadcrumbText() == "Browser");
    REQUIRE_FALSE (rig.panel.getBackTile().isVisible());
}
