#include "AirwindowsWiderModule.h"

namespace conduit
{

AirwindowsWiderModule::AirwindowsWiderModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Wider>(), staticModuleId, "Wider")
{
}

} // namespace conduit
