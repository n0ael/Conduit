#pragma once

#include "IOModule.h"

namespace conduit
{

//==============================================================================
/** Basisklasse der Netzwerk-I/O-Module (CLAUDE.md 4.1):
    OSC ↔ Ableton M4L, Link Audio Send/Receive. */
class NetworkIOModule : public IOModule
{
public:
    using IOModule::IOModule;
};

} // namespace conduit
