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
    struct Circle     { juce::Point<float> center; float radiusPx = 0.0f; };

    // Clang lehnt eine verschachtelte Config mit In-Class-Defaults als
    // Default-Argument ab ("needed within definition of enclosing class
    // ... outside of member functions") — Default-Ctor delegiert stattdessen
    // in der .cpp, wo RingTouchModel schon vollständig ist.
    RingTouchModel() noexcept;
    explicit RingTouchModel (const Config& cfg) noexcept;

    /** Neuer Touch: nächstes primäres Zentrum im grabRadius, das noch KEINEN
        Ring hat → Ring dieses Fingers; sonst neuer primärer Finger
        (Zentrum = pos, fix, wandert NICHT mit späterem Pitch-Bend-Gleiten). */
    DownResult onDown (uint32_t fingerId, juce::Point<float> pos) noexcept;

    /** Bewegung: nur Ring-Finger liefern hasSlide (Abstand→[0,1] zwischen
        min/maxRadius); primäre Bewegung interessiert dieses Model nicht. */
    MoveResult onMove (uint32_t fingerId, juce::Point<float> pos) noexcept;

    /** Abheben: Ring → wasRing/ringOwner; primär → wasPrimary/primaryFinger
        und löst eine evtl. Ring-Zuordnung (Ring wird inaktiv). */
    UpResult onUp (uint32_t fingerId) noexcept;

    std::vector<Circle> activeCircles() const; // Zeichnung; primär ohne Ring = minRadius
    void reset() noexcept;

private:
    struct PrimaryFinger
    {
        uint32_t id;
        juce::Point<float> center;
        uint32_t ringFinger = 0; // 0 = none
        float curRadiusPx = 0.0f;
    };

    Config config;
    std::vector<PrimaryFinger> primaries;
};

} // namespace conduit::grid
