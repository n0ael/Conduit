#include "AirwindowsPockey2Module.h"

namespace conduit
{

AirwindowsPockey2Module::AirwindowsPockey2Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Pockey2>(), staticModuleId, "Pockey2")
{
}

} // namespace conduit
