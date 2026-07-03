#include "AirwindowsTakeCareModule.h"

namespace conduit
{

AirwindowsTakeCareModule::AirwindowsTakeCareModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::TakeCare>(), staticModuleId, "TakeCare")
{
}

} // namespace conduit
