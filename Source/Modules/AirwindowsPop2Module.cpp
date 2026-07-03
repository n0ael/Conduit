#include "AirwindowsPop2Module.h"

namespace conduit
{

AirwindowsPop2Module::AirwindowsPop2Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Pop2>(), staticModuleId, "Pop2")
{
}

} // namespace conduit
