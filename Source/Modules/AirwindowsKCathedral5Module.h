#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/KCathedral5.h"

namespace conduit
{

//==============================================================================
/** kCathedral5 — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsKCathedral5Module final : public AirwindowsProcessorModule
{
public:
    AirwindowsKCathedral5Module();

    static constexpr const char* staticModuleId = "airwindows_kcathedral5";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsKCathedral5Module)
};

} // namespace conduit
