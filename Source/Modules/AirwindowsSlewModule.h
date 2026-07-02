#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Slew.h"

namespace conduit
{

//==============================================================================
/** Slew — Slew-Rate-Limiter / "Clamping".
    Siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSlewModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsSlewModule();

    static constexpr const char* staticModuleId = "airwindows_slew";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSlewModule)
};

} // namespace conduit
