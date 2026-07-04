#include "BrowserModel.h"

namespace conduit
{

namespace
{
    const juce::Identifier propSection ("section");

    constexpr auto sectionProjects = "projects";
    constexpr auto sectionAudio    = "audio";
    constexpr auto sectionModules  = "modules";
} // namespace

juce::String toString (BrowserContextProvider::Section section)
{
    switch (section)
    {
        case BrowserContextProvider::Section::projects: return sectionProjects;
        case BrowserContextProvider::Section::audio:    return sectionAudio;
        case BrowserContextProvider::Section::modules:  return sectionModules;
    }

    jassertfalse;
    return {};
}

BrowserContextProvider::Section sectionFromString (const juce::String& name)
{
    if (name == sectionAudio)   return BrowserContextProvider::Section::audio;
    if (name == sectionModules) return BrowserContextProvider::Section::modules;
    return BrowserContextProvider::Section::projects;
}

//==============================================================================
BrowserModel::BrowserModel (ModuleFactory& factoryToUse,
                            BrowserContextProvider& contextToUse)
    : factory (factoryToUse), context (contextToUse)
{
    juce::ignoreUnused (factory);  // Modul-Zeilen folgen in M2

    context.onContextChanged = [this] { handleContextChanged(); };
    rebuildRows();
}

//==============================================================================
void BrowserModel::openStartSection()
{
    openSection (context.startSection());
}

void BrowserModel::openSection (BrowserContextProvider::Section section)
{
    if (! context.isSectionVisible (section))
    {
        // Unsichtbarer Bereich (MODULE außerhalb der Device-Page) →
        // Übersicht statt Sackgasse
        setCurrentSection ({});
        return;
    }

    setCurrentSection (toString (section));
}

void BrowserModel::goBack()
{
    if (! canGoBack())
        return;

    setCurrentSection ({});
}

bool BrowserModel::canGoBack() const
{
    return currentSectionName().isNotEmpty();
}

juce::String BrowserModel::breadcrumbText() const
{
    const auto section = currentSectionName();

    if (section == sectionProjects) return "PROJEKTE";
    if (section == sectionAudio)    return "AUDIO";
    if (section == sectionModules)  return "MODULE";

    return "Browser";
}

bool BrowserModel::activateRow (int index)
{
    if (index < 0 || index >= (int) visibleRows.size())
        return false;

    const auto& row = visibleRows[(size_t) index];

    switch (row.kind)
    {
        case Row::Kind::section:
            openSection (sectionFromString (row.id));
            return true;

        // Branch-/Kategorie-Navigation folgt in M2
        case Row::Kind::branch:
        case Row::Kind::category:
        case Row::Kind::module:
        case Row::Kind::file:
        case Row::Kind::action:
        case Row::Kind::hint:
            return false;
    }

    jassertfalse;
    return false;
}

//==============================================================================
void BrowserModel::handleContextChanged()
{
    // Aktive Section unsichtbar geworden (Device-Page verlassen, MODULE
    // offen) → Startbereich der neuen Page; sonst Ansicht behalten
    const auto section = currentSectionName();

    if (section.isNotEmpty()
        && ! context.isSectionVisible (sectionFromString (section)))
    {
        openStartSection();
        return;
    }

    rebuildRows();  // Übersicht kann Zeilen dazubekommen/verlieren
}

void BrowserModel::rebuildRows()
{
    visibleRows.clear();

    const auto section = currentSectionName();

    if (section.isEmpty())
    {
        // Übersicht: die sichtbaren Hauptbereiche in fixer Reihenfolge
        visibleRows.push_back ({ Row::Kind::section, Icon::projects,
                                 "PROJEKTE", sectionProjects });
        visibleRows.push_back ({ Row::Kind::section, Icon::audio,
                                 "AUDIO", sectionAudio });

        if (context.isSectionVisible (BrowserContextProvider::Section::modules))
            visibleRows.push_back ({ Row::Kind::section, Icon::none,
                                     "MODULE", sectionModules });
    }
    else if (section == sectionModules)
    {
        // Ebene 1: die beiden Modul-Äste (Navigation folgt in M2)
        visibleRows.push_back ({ Row::Kind::branch, Icon::cvControl,
                                 "CV/Control", "cv" });
        visibleRows.push_back ({ Row::Kind::branch, Icon::audioFx,
                                 "AudioFX", "fx" });
    }
    else
    {
        // PROJEKTE/AUDIO: Datenanbindung folgt in M6
        visibleRows.push_back ({ Row::Kind::hint, Icon::none,
                                 juce::String::fromUTF8 ("Inhalte folgen …"), {} });
    }

    if (onRowsChanged != nullptr)
        onRowsChanged();
}

juce::String BrowserModel::currentSectionName() const
{
    return state.getProperty (propSection).toString();
}

void BrowserModel::setCurrentSection (const juce::String& sectionName)
{
    // nullptr-UndoManager: Browser-Zustand ist nie undo-fähig (Kopf-Doku)
    state.setProperty (propSection, sectionName, nullptr);
    rebuildRows();
}

} // namespace conduit
