#include "AirwindowsParametricModule.h"

namespace conduit
{

AirwindowsParametricModule::AirwindowsParametricModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::Parametric>(), staticModuleId, "Parametric")
{
}

} // namespace conduit
