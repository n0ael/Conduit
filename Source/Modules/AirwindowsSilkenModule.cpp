#include "AirwindowsSilkenModule.h"

namespace conduit
{

AirwindowsSilkenModule::AirwindowsSilkenModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Silken>(), staticModuleId, "Silken")
{
}

} // namespace conduit
