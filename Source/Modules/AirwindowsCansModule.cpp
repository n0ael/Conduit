#include "AirwindowsCansModule.h"

namespace conduit
{

AirwindowsCansModule::AirwindowsCansModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Cans>(), staticModuleId, "Cans")
{
}

} // namespace conduit
