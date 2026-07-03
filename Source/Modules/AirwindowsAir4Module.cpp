#include "AirwindowsAir4Module.h"

namespace conduit
{

AirwindowsAir4Module::AirwindowsAir4Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Air4>(), staticModuleId, "Air4")
{
}

} // namespace conduit
