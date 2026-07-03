#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/KWoodRoom.h"

namespace conduit
{

//==============================================================================
/** kWoodRoom — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsKWoodRoomModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsKWoodRoomModule();

    static constexpr const char* staticModuleId = "airwindows_kwoodroom";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsKWoodRoomModule)
};

} // namespace conduit
