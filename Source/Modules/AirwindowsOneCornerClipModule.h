#pragma once

#include "AirwindowsProcessorModule.h"
#include "DSP/Airwindows/Plugins/OneCornerClip.h"

namespace conduit
{

//==============================================================================
/** OneCornerClip — siehe AirwindowsProcessorModule für die gemeinsame Wrapper-Logik. */
class AirwindowsOneCornerClipModule final : public AirwindowsProcessorModule
{
public:
    AirwindowsOneCornerClipModule();

    static constexpr const char* staticModuleId = "airwindows_onecornerclip";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirwindowsOneCornerClipModule)
};

} // namespace conduit
