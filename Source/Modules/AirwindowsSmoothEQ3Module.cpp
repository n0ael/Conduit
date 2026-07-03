#include "AirwindowsSmoothEQ3Module.h"

namespace conduit
{

AirwindowsSmoothEQ3Module::AirwindowsSmoothEQ3Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::SmoothEQ3>(), staticModuleId, "SmoothEQ3")
{
}

} // namespace conduit
