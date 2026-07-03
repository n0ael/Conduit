#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/VerbTiny.h"

namespace conduit
{

//==============================================================================
/** VerbTiny — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsVerbTinyModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsVerbTinyModule();

    static constexpr const char* staticModuleId = "airwindows_verbtiny";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsVerbTinyModule)
};

} // namespace conduit
