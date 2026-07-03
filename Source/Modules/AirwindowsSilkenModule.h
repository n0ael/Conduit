#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Silken.h"

namespace conduit
{

//==============================================================================
/** Silken — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsSilkenModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsSilkenModule();

    static constexpr const char* staticModuleId = "airwindows_silken";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsSilkenModule)
};

} // namespace conduit
