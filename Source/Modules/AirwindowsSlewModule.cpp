#include "AirwindowsSlewModule.h"

namespace conduit
{

AirwindowsSlewModule::AirwindowsSlewModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Slew>(), staticModuleId, "Slew")
{
}

} // namespace conduit
