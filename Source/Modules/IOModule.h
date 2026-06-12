#pragma once

#include "ConduitModule.h"

namespace conduit
{

//==============================================================================
/** Basisklasse aller I/O-Module (CLAUDE.md 4.1):
    HardwareIO (ES-3, ES-5, ESX-8GT, ESX-8CV), NetworkIO (OSC, Link Audio). */
class IOModule : public ConduitModule
{
public:
    using ConduitModule::ConduitModule;

    [[nodiscard]] ModuleType getType() const override { return ModuleType::io; }
};

} // namespace conduit
