#include "AirwindowsVerbTinyModule.h"

namespace conduit
{

AirwindowsVerbTinyModule::AirwindowsVerbTinyModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::VerbTiny>(), staticModuleId, "VerbTiny")
{
}

} // namespace conduit
