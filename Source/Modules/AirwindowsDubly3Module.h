#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Dubly3.h"

namespace conduit
{

//==============================================================================
/** Dubly3 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsDubly3Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsDubly3Module();

    static constexpr const char* staticModuleId = "airwindows_dubly3";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsDubly3Module)
};

} // namespace conduit
