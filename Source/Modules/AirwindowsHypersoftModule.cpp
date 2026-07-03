#include "AirwindowsHypersoftModule.h"

namespace conduit
{

AirwindowsHypersoftModule::AirwindowsHypersoftModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Hypersoft>(), staticModuleId, "Hypersoft")
{
}

} // namespace conduit
