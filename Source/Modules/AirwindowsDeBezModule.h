#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/DeBez.h"

namespace conduit
{

//==============================================================================
/** DeBez — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDeBezModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsDeBezModule();

    static constexpr const char* staticModuleId = "airwindows_debez";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDeBezModule)
};

} // namespace conduit
