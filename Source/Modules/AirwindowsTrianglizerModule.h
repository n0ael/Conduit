#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/Trianglizer.h"

namespace conduit
{

//==============================================================================
/** Trianglizer — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsTrianglizerModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsTrianglizerModule();

    static constexpr const char* staticModuleId = "airwindows_trianglizer";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsTrianglizerModule)
};

} // namespace conduit
