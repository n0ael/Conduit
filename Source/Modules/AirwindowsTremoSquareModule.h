#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/TremoSquare.h"

namespace conduit
{

//==============================================================================
/** TremoSquare — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsTremoSquareModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsTremoSquareModule();

    static constexpr const char* staticModuleId = "airwindows_tremosquare";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsTremoSquareModule)
};

} // namespace conduit
