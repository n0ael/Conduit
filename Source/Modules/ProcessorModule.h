#pragma once

#include "ConduitModule.h"

namespace conduit
{

//==============================================================================
/** Basisklasse aller Processor-Module (CLAUDE.md 4.1):
    Gate, EQ, Compressor, PluginModule (CLAP-Host, v2.x). */
class ProcessorModule : public ConduitModule
{
public:
    using ConduitModule::ConduitModule;

    [[nodiscard]] ModuleType getType() const override { return ModuleType::processor; }
};

} // namespace conduit
