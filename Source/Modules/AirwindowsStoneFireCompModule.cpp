#include "AirwindowsStoneFireCompModule.h"

namespace conduit
{

AirwindowsStoneFireCompModule::AirwindowsStoneFireCompModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::StoneFireComp>(), staticModuleId, "StoneFireComp")
{
}

} // namespace conduit
