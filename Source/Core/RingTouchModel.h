#pragma once
#include <cstdint>
#include <vector>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h> // juce::Point lebt in juce_graphics, nicht juce_core

namespace conduit::grid
{

/** Ordnet Touch-Finger primären Fingern (Note) und Ring-Fingern (zweite
    Achse) zu. Pixel-Koordinaten. Message-Thread, kein RT. Testbar. */
class RingTouchModel
{
public:
    struct Config
    {
        float grabRadiusPx = 90.0f;   // Down näher als dies an einem primären Zentrum → Ring
        float minRadiusPx  = 40.0f;   // Slide = 0
        float maxRadiusPx  = 220.0f;  // Slide = 1
    };

    enum class TouchKind { Primary, Ring };
    struct DownResult { TouchKind kind; uint32_t ringOwner = 0; };
    struct MoveResult { bool hasSlide = false; uint32_t owner = 0; float slide01 = 0.0f; };
    struct UpResult   { bool wasPrimary = false; bool wasRing = false;
                        uint32_t primaryFinger = 0; uint32_t ringOwner = 0; };
    /** hasOrbit/orbitPos: Position des "Planeten" (Ring-Finger, live oder
        eingefroren) — nur gültig, wenn ein Ring je aktiv war. */
    struct Circle     { juce::Point<float> center; float radiusPx = 0.0f;
                        bool hasOrbit = false; juce::Point<float> orbitPos; };

    // Clang lehnt eine verschachtelte Config mit In-Class-Defaults als
    // Default-Argument ab ("needed within definition of enclosing class
    // ... outside of member functions") — Default-Ctor delegiert stattdessen
    // in der .cpp, wo RingTouchModel schon vollständig ist.
    RingTouchModel() noexcept;
    explicit RingTouchModel (const Config& cfg) noexcept;

    /** Neuer Touch: nächster Greifpunkt im grabRadius, der noch KEINEN Ring
        hat → Ring dieses Fingers; sonst neuer primärer Finger. Greifpunkt ist
        die aktuelle Mond-Position, falls dieser primäre Finger schon einmal
        einen Ring hatte (Wiederaufnahme der eingefrorenen Umlaufbahn dort,
        wo sie steht — kann weit vom Zentrum entfernt sein), sonst das
        Zentrum selbst (Erstgriff). */
    DownResult onDown (uint32_t fingerId, juce::Point<float> pos) noexcept;

    /** Bewegung: Ring-Finger liefern hasSlide (Betrag des Offsets zum
        AKTUELLEN Zentrum→[0,1] zwischen min/maxRadius) und erfassen den
        Offset relativ zum Zentrum neu. Primäre Bewegung liefert kein Slide;
        das Zentrum wandert mit dem Finger, der Ring-OFFSET bleibt dabei
        UNVERÄNDERT — der Mond "klebt" relativ zur Sonne und wandert mit ihr
        mit (User-Entscheidung 06.07.2026: nur ein aktiv bewegter Ring-Finger
        darf die Umlaufbahn selbst verändern). */
    MoveResult onMove (uint32_t fingerId, juce::Point<float> pos) noexcept;

    /** Abheben: Ring → wasRing/ringOwner, Ring wird inaktiv (ringFinger = 0,
        erneut greifbar) — Mond-Orbit-Verhalten: der Ring-Offset (Radius +
        Richtung relativ zum Zentrum) bleibt eingefroren, KEIN Reset. Primär →
        wasPrimary/primaryFinger, der Finger wird entfernt (löst automatisch
        auch eine evtl. Ring-Zuordnung). */
    UpResult onUp (uint32_t fingerId) noexcept;

    std::vector<Circle> activeCircles() const; // Zeichnung; ohne Ring-Historie = minRadius
    void reset() noexcept;

    /** Ruheradius (Sonne/Planet-Größe fürs UI, CLAUDE.md-fremd — reiner
        Zeichen-Parameter, keine Logik). */
    [[nodiscard]] float restRadiusPx() const noexcept { return config.minRadiusPx; }

private:
    struct PrimaryFinger
    {
        uint32_t id;
        juce::Point<float> center;      // live, folgt dem primären Finger
        uint32_t ringFinger = 0;        // 0 = none
        juce::Point<float> ringOffset;  // Ring-Position RELATIV zu center (friert/wandert mit)
        bool hasOrbit = false;          // true, sobald je ein Ring angedockt hat
        float curRadiusPx = 0.0f;       // = ringOffset.getDistanceFromOrigin(), gecached
    };

    Config config;
    std::vector<PrimaryFinger> primaries;
};

} // namespace conduit::grid
