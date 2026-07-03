#include "AirwindowsTube2Module.h"

namespace conduit
{

AirwindowsTube2Module::AirwindowsTube2Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Tube2>(), staticModuleId, "Tube2")
{
}

} // namespace conduit
