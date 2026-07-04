#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "ConduitModule.h"

namespace conduit
{

//==============================================================================
/**
    Statische Browser-Metadaten eines registrierten Moduls — die Factory ist
    die Single Source of Truth für „was existiert" (Registrierung ohne
    Descriptor gibt es nicht, damit Browser und Factory nie auseinanderlaufen).

    branch/category bilden die zwei Navigationsebenen des Browser-Panels:
      cvControl → Kategorien wie "LFO", "Sequencer", "Analyse", "I/O", "Utility"
      audioFx   → Kategorien aus der AirwindowsRegistry (4.6-Chassis-Module)

    tags: lowercase Suchbegriffe (Browser-Suchindex). Nur Message Thread.
*/
struct ModuleDescriptor
{
    juce::String id;            // factoryKey (== staticModuleId)
    juce::String displayName;

    enum class Branch { cvControl, audioFx };
    Branch branch = Branch::cvControl;

    juce::String category;
    juce::StringArray tags;
};

//==============================================================================
/**
    Erzeugt Modul-Instanzen aus persistierten moduleIds (Schema 6.2).

    Der GraphManager materialisiert darüber Module beim Graph-Swap —
    derselbe Pfad gilt für User-Add, Undo/Redo und Preset-Load.

    Nur Message Thread.
*/
class ModuleFactory final
{
public:
    using Creator = std::function<std::unique_ptr<ConduitModule>()>;

    ModuleFactory() = default;

    void registerModule (ModuleDescriptor descriptor, Creator creator);

    [[nodiscard]] bool isRegistered (const juce::String& moduleId) const;

    /** nullptr, wenn die moduleId unbekannt ist — der GraphManager setzt
        dann nodeError im ValueTree. */
    [[nodiscard]] std::unique_ptr<ConduitModule> create (const juce::String& moduleId) const;

    /** Alle Descriptors, nach displayName sortiert (Browser-Anzeige). */
    [[nodiscard]] std::vector<ModuleDescriptor> getDescriptors() const;

    /** Descriptor eines einzelnen Moduls; id leer, wenn unbekannt. */
    [[nodiscard]] ModuleDescriptor getDescriptor (const juce::String& moduleId) const;

private:
    struct Entry
    {
        ModuleDescriptor descriptor;
        Creator creator;
    };

    std::map<juce::String, Entry> entries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleFactory)
};

/** Registriert alle eingebauten Module. */
void registerDefaultModules (ModuleFactory& factory);

} // namespace conduit
