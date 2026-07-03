#include "AirwindowsVibratoModule.h"

namespace conduit
{

AirwindowsVibratoModule::AirwindowsVibratoModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Vibrato>(), staticModuleId, "Vibrato")
{
}

} // namespace conduit
