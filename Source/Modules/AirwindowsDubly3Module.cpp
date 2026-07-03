#include "AirwindowsDubly3Module.h"

namespace conduit
{

AirwindowsDubly3Module::AirwindowsDubly3Module()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Dubly3>(), staticModuleId, "Dubly3")
{
}

} // namespace conduit
