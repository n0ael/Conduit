#include "AirwindowsSingleEndedTriodeModule.h"

namespace conduit
{

AirwindowsSingleEndedTriodeModule::AirwindowsSingleEndedTriodeModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::SingleEndedTriode>(), staticModuleId, "SingleEndedTriode")
{
}

} // namespace conduit
