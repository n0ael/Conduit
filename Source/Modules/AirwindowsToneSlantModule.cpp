#include "AirwindowsToneSlantModule.h"

namespace conduit
{

AirwindowsToneSlantModule::AirwindowsToneSlantModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::ToneSlant>(), staticModuleId, "ToneSlant")
{
}

} // namespace conduit
