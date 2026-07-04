/* ========================================
 *  AirwindowsRegistry.h — Katalog der portierten Airwindows-Effekte
 *  DSP-Originale: Chris Johnson / Airwindows (MIT-Lizenz)
 * ======================================== */

#pragma once

#include <memory>
#include <span>
#include <string_view>

#include "DSP/Airwindows/AirwindowsPlugin.h"

namespace conduit::airwindows
{

/** Ein Katalog-Eintrag: stabiler Schlüssel (spätere factoryId-Basis),
    Anzeigename, Browser-Metadaten und Factory-Funktion.

    category/tags sind die Single Source of Truth für die AudioFX-
    Kategorisierung des Browser-Panels (ModuleFactory-Descriptors lesen
    sie hier aus, nie duplizieren). Kategorie-Menge fix: "Dynamics",
    "Filter/EQ", "Distortion/Saturation", "Lo-Fi/Tape", "Modulation",
    "Console", "Reverb/Delay", "Utility". tags: lowercase, Leerzeichen-
    getrennt, für die Browser-Suche. */
struct RegistryEntry
{
    const char* id;
    const char* name;
    const char* category;
    const char* tags;
    std::unique_ptr<AirwindowsPlugin> (*create)();
};

/** Alle portierten Effekte — statische Daten, von jedem Thread lesbar. */
std::span<const RegistryEntry> getRegisteredPlugins() noexcept;

/** [Message Thread] Instanz erzeugen; nullptr bei unbekannter id. */
std::unique_ptr<AirwindowsPlugin> createPlugin (std::string_view id);

} // namespace conduit::airwindows
