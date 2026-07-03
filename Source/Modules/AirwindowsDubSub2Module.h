#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/DubSub2.h"

namespace conduit
{

//==============================================================================
/** DubSub2 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDubSub2Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsDubSub2Module();

    static constexpr const char* staticModuleId = "airwindows_dubsub2";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDubSub2Module)
};

} // namespace conduit
