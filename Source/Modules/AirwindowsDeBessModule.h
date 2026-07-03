#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/DeBess.h"

namespace conduit
{

//==============================================================================
/** DeBess — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDeBessModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsDeBessModule();

    static constexpr const char* staticModuleId = "airwindows_debess";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDeBessModule)
};

} // namespace conduit
