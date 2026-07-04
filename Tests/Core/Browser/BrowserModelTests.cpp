#include <catch2/catch_test_macros.hpp>

#include "Core/Browser/BrowserModel.h"
#include "DSP/Airwindows/AirwindowsRegistry.h"

namespace
{
using Section = conduit::BrowserContextProvider::Section;
using Row     = conduit::BrowserModel::Row;

constexpr int pageMixer  = 1;
constexpr int pageDevice = 3;

struct ModelRig
{
    ModelRig() { conduit::registerDefaultModules (factory); }

    conduit::ModuleFactory factory;
    conduit::BrowserContextProvider context;
    conduit::BrowserModel model { factory, context };
};

bool hasRowWithLabel (const std::vector<Row>& rows, const juce::String& label)
{
    return std::any_of (rows.begin(), rows.end(),
                        [&label] (const Row& row) { return row.label == label; });
}
} // namespace

//==============================================================================
TEST_CASE ("Browser-Modell: Übersicht zeigt die Bereiche kontextabhängig", "[browser]")
{
    ModelRig rig;

    // Device-Page (Default): alle drei Bereiche
    REQUIRE (rig.model.rows().size() == 3);
    REQUIRE (hasRowWithLabel (rig.model.rows(), "PROJEKTE"));
    REQUIRE (hasRowWithLabel (rig.model.rows(), "AUDIO"));
    REQUIRE (hasRowWithLabel (rig.model.rows(), "MODULE"));

    // andere Page: MODULE verschwindet aus der Übersicht
    rig.context.setActivePage (pageMixer);
    REQUIRE (rig.model.rows().size() == 2);
    REQUIRE_FALSE (hasRowWithLabel (rig.model.rows(), "MODULE"));
}

TEST_CASE ("Browser-Modell: Startbereich + Zurück-Navigation", "[browser]")
{
    ModelRig rig;

    // Öffnen auf der Device-Page → direkt in MODULE (User-Entscheidung)
    rig.model.openStartSection();
    REQUIRE (rig.model.breadcrumbText() == "MODULE");
    REQUIRE (rig.model.canGoBack());

    // MODULE-Wurzel: die beiden Äste
    REQUIRE (rig.model.rows().size() == 2);
    REQUIRE (hasRowWithLabel (rig.model.rows(), "CV/Control"));
    REQUIRE (hasRowWithLabel (rig.model.rows(), "AudioFX"));

    // Zurück-Pfeil führt zur Bereichsübersicht
    rig.model.goBack();
    REQUIRE (rig.model.breadcrumbText() == "Browser");
    REQUIRE_FALSE (rig.model.canGoBack());
}

TEST_CASE ("Browser-Modell: Klick auf Bereichs-Zeile navigiert", "[browser]")
{
    ModelRig rig;

    // Zeile 0 = PROJEKTE (fixe Reihenfolge)
    REQUIRE (rig.model.activateRow (0));
    REQUIRE (rig.model.breadcrumbText() == "PROJEKTE");

    // Hinweis-Zeile (Inhalte folgen) konsumiert den Klick nicht
    REQUIRE_FALSE (rig.model.activateRow (0));
}

TEST_CASE ("Browser-Modell: Kontextwechsel verlässt unsichtbare Bereiche", "[browser]")
{
    ModelRig rig;

    rig.model.openSection (Section::modules);
    REQUIRE (rig.model.breadcrumbText() == "MODULE");

    // Device-Page verlassen → MODULE unsichtbar → Startbereich der neuen Page
    rig.context.setActivePage (pageMixer);
    REQUIRE (rig.model.breadcrumbText() == "PROJEKTE");

    // PROJEKTE bleibt bei weiteren Wechseln stehen (global sichtbar)
    rig.context.setActivePage (pageDevice);
    REQUIRE (rig.model.breadcrumbText() == "PROJEKTE");
}

//==============================================================================
TEST_CASE ("Modul-Descriptors: jeder registrierte Key trägt Branch + Kategorie",
           "[browser][factory]")
{
    conduit::ModuleFactory factory;
    conduit::registerDefaultModules (factory);

    const auto descriptors = factory.getDescriptors();

    // 57 Airwindows + 6 CV/Control-Module
    REQUIRE (descriptors.size() == 63);

    for (const auto& descriptor : descriptors)
    {
        INFO ("Modul " << descriptor.id);
        REQUIRE (descriptor.id.isNotEmpty());
        REQUIRE (descriptor.displayName.isNotEmpty());
        REQUIRE (descriptor.category.isNotEmpty());
        REQUIRE (factory.isRegistered (descriptor.id));
    }

    // Sortierung: displayName aufsteigend (Browser-Anzeige)
    for (size_t i = 1; i < descriptors.size(); ++i)
        REQUIRE (descriptors[i - 1].displayName
                     .compareIgnoreCase (descriptors[i].displayName) <= 0);
}

TEST_CASE ("Airwindows-Registry: Kategorien aus der erlaubten Menge, Tags gesetzt",
           "[browser][factory]")
{
    const juce::StringArray allowed { "Dynamics", "Filter/EQ", "Distortion/Saturation",
                                      "Lo-Fi/Tape", "Modulation", "Console",
                                      "Reverb/Delay", "Utility" };

    int count = 0;
    for (const auto& entry : conduit::airwindows::getRegisteredPlugins())
    {
        INFO ("Plugin " << entry.id);
        REQUIRE (allowed.contains (juce::String (entry.category)));
        REQUIRE (juce::String (entry.tags).isNotEmpty());
        ++count;
    }

    REQUIRE (count == 57);
}

TEST_CASE ("Modul-Descriptors: Airwindows-Wrapper spiegeln die Registry",
           "[browser][factory]")
{
    conduit::ModuleFactory factory;
    conduit::registerDefaultModules (factory);

    for (const auto& entry : conduit::airwindows::getRegisteredPlugins())
    {
        const auto factoryKey = "airwindows_" + juce::String (entry.id);
        const auto descriptor = factory.getDescriptor (factoryKey);

        INFO ("Plugin " << entry.id);
        REQUIRE (descriptor.id == factoryKey);
        REQUIRE (descriptor.displayName == juce::String (entry.name));
        REQUIRE (descriptor.branch == conduit::ModuleDescriptor::Branch::audioFx);
        REQUIRE (descriptor.category == juce::String (entry.category));
        REQUIRE (! descriptor.tags.isEmpty());
    }
}
