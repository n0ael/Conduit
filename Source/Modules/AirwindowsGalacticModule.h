#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Galactic.h"

namespace conduit
{

//==============================================================================
/** Galactic — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsGalacticModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsGalacticModule();

    static constexpr const char* staticModuleId = "airwindows_galactic";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsGalacticModule)
};

} // namespace conduit
