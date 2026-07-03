#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/DigitalBlack.h"

namespace conduit
{

//==============================================================================
/** DigitalBlack — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDigitalBlackModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsDigitalBlackModule();

    static constexpr const char* staticModuleId = "airwindows_digitalblack";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDigitalBlackModule)
};

} // namespace conduit
