#include "AirwindowsFlutter2Module.h"

namespace conduit
{

AirwindowsFlutter2Module::AirwindowsFlutter2Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Flutter2>(), staticModuleId, "Flutter2")
{
}

} // namespace conduit
