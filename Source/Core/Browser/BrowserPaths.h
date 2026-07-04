#pragma once

#include <juce_core/juce_core.h>

namespace conduit::browser_paths
{

//==============================================================================
/** Default-Verzeichnisse der Browser-Bereiche (User-Entscheidung
    04.07.2026: alles unter Dokumente/Conduit; Captures kommen aus dem
    bestehenden CaptureSettings-Exportordner und stehen bewusst NICHT
    hier). Konfigurierbare Pfade folgen später als Settings — bis dahin
    ist DIES die eine Stelle. Verzeichnisse werden bei Bedarf angelegt. */

[[nodiscard]] juce::File projectsDirectory();   // Dokumente/Conduit (*.conduit)
[[nodiscard]] juce::File loopsDirectory();      // Dokumente/Conduit/Loops
[[nodiscard]] juce::File oneShotsDirectory();   // Dokumente/Conduit/One-Shots

} // namespace conduit::browser_paths
