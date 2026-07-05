#pragma once

#include <string_view>

namespace conduit
{

//==============================================================================
/**
    App-weites Launch-Quantisierungs-Raster (CLAUDE.md 4.5) — EIN Enum für
    alle Module, die quantisiert starten/stoppen (Looper-Clips, später
    Sequencer/Euclid), damit überall identisch gerastert wird.

    Bewusst JUCE-frei (Muster LooperMath): Engine, UI und Tests teilen sich
    dieselbe Tabelle. Die Grid-Erkennung selbst (floor-Überquerung pro Block)
    lebt beim Konsumenten — hier stehen nur Werte, Namen und Persist-Keys.

    qBeats == 0 bedeutet "sofort am Blockanfang" (Grid Off).
*/
enum class LaunchQuant : int
{
    off = 0,
    bars8,
    bars4,
    bars2,
    bar1,
    half,
    quarter,
    eighth,
    sixteenth,
    thirtySecond,
};

constexpr int numLaunchQuants = 10;

/** Rasterweite in Beats (Viertelnoten, Link-Achse; 1 Takt = 4 Beats). */
[[nodiscard]] constexpr double launchQuantBeats (LaunchQuant q) noexcept
{
    switch (q)
    {
        case LaunchQuant::off:          return 0.0;
        case LaunchQuant::bars8:        return 32.0;
        case LaunchQuant::bars4:        return 16.0;
        case LaunchQuant::bars2:        return 8.0;
        case LaunchQuant::bar1:         return 4.0;
        case LaunchQuant::half:         return 2.0;
        case LaunchQuant::quarter:      return 1.0;
        case LaunchQuant::eighth:       return 0.5;
        case LaunchQuant::sixteenth:    return 0.25;
        case LaunchQuant::thirtySecond: return 0.125;
    }
    return 0.0;
}

/** UI-Name (Ableton-Konvention). */
[[nodiscard]] constexpr const char* launchQuantName (LaunchQuant q) noexcept
{
    switch (q)
    {
        case LaunchQuant::off:          return "None";
        case LaunchQuant::bars8:        return "8 Bars";
        case LaunchQuant::bars4:        return "4 Bars";
        case LaunchQuant::bars2:        return "2 Bars";
        case LaunchQuant::bar1:         return "1 Bar";
        case LaunchQuant::half:         return "1/2";
        case LaunchQuant::quarter:      return "1/4";
        case LaunchQuant::eighth:       return "1/8";
        case LaunchQuant::sixteenth:    return "1/16";
        case LaunchQuant::thirtySecond: return "1/32";
    }
    return "None";
}

/** Stabiler Persist-Key (Settings-Dateien) — NIE umbenennen. */
[[nodiscard]] constexpr const char* launchQuantKey (LaunchQuant q) noexcept
{
    switch (q)
    {
        case LaunchQuant::off:          return "off";
        case LaunchQuant::bars8:        return "8bars";
        case LaunchQuant::bars4:        return "4bars";
        case LaunchQuant::bars2:        return "2bars";
        case LaunchQuant::bar1:         return "1bar";
        case LaunchQuant::half:         return "1/2";
        case LaunchQuant::quarter:      return "1/4";
        case LaunchQuant::eighth:       return "1/8";
        case LaunchQuant::sixteenth:    return "1/16";
        case LaunchQuant::thirtySecond: return "1/32";
    }
    return "off";
}

/** Persist-Key → Enum; unbekannte Keys fallen auf `fallback`. */
[[nodiscard]] constexpr LaunchQuant launchQuantFromKey (std::string_view key,
                                                        LaunchQuant fallback) noexcept
{
    for (int i = 0; i < numLaunchQuants; ++i)
    {
        const auto q = static_cast<LaunchQuant> (i);
        if (key == launchQuantKey (q))
            return q;
    }
    return fallback;
}

} // namespace conduit
