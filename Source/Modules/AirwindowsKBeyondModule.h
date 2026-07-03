#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/KBeyond.h"

namespace conduit
{

//==============================================================================
/** kBeyond — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsKBeyondModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsKBeyondModule();

    static constexpr const char* staticModuleId = "airwindows_kbeyond";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsKBeyondModule)
};

} // namespace conduit
