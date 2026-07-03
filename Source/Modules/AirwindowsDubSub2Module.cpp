#include "AirwindowsDubSub2Module.h"

namespace conduit
{

AirwindowsDubSub2Module::AirwindowsDubSub2Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::DubSub2>(), staticModuleId, "DubSub2")
{
}

} // namespace conduit
