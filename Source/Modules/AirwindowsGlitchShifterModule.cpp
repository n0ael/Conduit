#include "AirwindowsGlitchShifterModule.h"

namespace conduit
{

AirwindowsGlitchShifterModule::AirwindowsGlitchShifterModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::GlitchShifter>(), staticModuleId, "GlitchShifter")
{
}

} // namespace conduit
