#include "AirwindowsDeBezModule.h"

namespace conduit
{

AirwindowsDeBezModule::AirwindowsDeBezModule()
    : AirwindowsProcessorModule (std::make_unique<airwindows::DeBez>(), staticModuleId, "DeBez")
{
}

} // namespace conduit
