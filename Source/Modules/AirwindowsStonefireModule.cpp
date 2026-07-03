#include "AirwindowsStonefireModule.h"

namespace conduit
{

AirwindowsStonefireModule::AirwindowsStonefireModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Stonefire>(), staticModuleId, "Stonefire")
{
}

} // namespace conduit
