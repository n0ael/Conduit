#include "AirwindowsPearEQModule.h"

namespace conduit
{

AirwindowsPearEQModule::AirwindowsPearEQModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::PearEQ>(), staticModuleId, "PearEQ")
{
}

} // namespace conduit
