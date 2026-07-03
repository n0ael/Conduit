#include "AirwindowsDeBessModule.h"

namespace conduit
{

AirwindowsDeBessModule::AirwindowsDeBessModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::DeBess>(), staticModuleId, "DeBess")
{
}

} // namespace conduit
