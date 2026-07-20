#pragma once

#include <cstdint>

namespace conduit
{

//==============================================================================
/**
    Mixin-Interface: Zufalls-Parameter (CLAUDE.md 4.2) → Audio Thread,
    Seed-Updates → Message Thread.

    RNG-Vorgabe ist der §3.1-inline-LCG (state = 1664525*state +
    1013904223) mit modul-eigenem uint32-State — kein globaler/geteilter
    Zufall, keine rand()/<random>-Engines. Seed-Injektion auf dem Message
    Thread VOR der Graph-Aufnahme (setRandomSeed, Muster IClockSlave):
    der GraphManager injiziert beim Materialisieren einen deterministischen
    Seed aus der nodeUuid — Patterns sind damit pro Node reproduzierbar,
    Tests setzen den Seed direkt. Nutzung ausschließlich [Audio Thread],
    deterministisch pro Seed.
*/
class IStochastic
{
public:
    virtual ~IStochastic() = default;

    /** Message Thread, vor der Graph-Aufnahme (oder bei gestopptem Audio). */
    virtual void setRandomSeed (std::uint64_t seed) noexcept = 0;
};

} // namespace conduit
