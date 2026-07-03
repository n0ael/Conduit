#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/TapeDust.h"

namespace conduit
{

//==============================================================================
/** TapeDust — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsTapeDustModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsTapeDustModule();

    static constexpr const char* staticModuleId = "airwindows_tapedust";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsTapeDustModule)
};

} // namespace conduit
