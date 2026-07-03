#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/PointyGuitar.h"

namespace conduit
{

//==============================================================================
/** PointyGuitar — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsPointyGuitarModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsPointyGuitarModule();

    static constexpr const char* staticModuleId = "airwindows_pointyguitar";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsPointyGuitarModule)
};

} // namespace conduit
