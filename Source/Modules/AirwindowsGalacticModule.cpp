#include "AirwindowsGalacticModule.h"

namespace conduit
{

AirwindowsGalacticModule::AirwindowsGalacticModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Galactic>(), staticModuleId, "Galactic")
{
}

} // namespace conduit
