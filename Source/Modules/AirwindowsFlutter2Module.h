#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Flutter2.h"

namespace conduit
{

//==============================================================================
/** Flutter2 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsFlutter2Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsFlutter2Module();

    static constexpr const char* staticModuleId = "airwindows_flutter2";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsFlutter2Module)
};

} // namespace conduit
