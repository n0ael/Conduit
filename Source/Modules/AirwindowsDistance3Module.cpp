#include "AirwindowsDistance3Module.h"

namespace conduit
{

AirwindowsDistance3Module::AirwindowsDistance3Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Distance3>(), staticModuleId, "Distance3")
{
}

} // namespace conduit
